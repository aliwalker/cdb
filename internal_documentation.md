# Cdb Internal Documentation

Cdb is implemented on top of [rpclib](http://rpclib.net/) and [leveldb](https://github.com/google/leveldb).

## RPCs

We've mentioned that the coordinator makes RPCs to participants in the intro. There're a couple of RPCs that the system relies on to function "correctly". Nonetheless, be warned that there're some ad-hoc mechanisms employed, which means the whole consensus algorithm is rather unverified. 

### GET RPC
Simple as it is. It returns a string representing the value associated with the key. No 2PC is needed. See [this line](./src/participant.cpp#L15)

### SET_PREPARE RPC
The 1st phase of 2PC. It takes a `set_command` object as its parameter. It returns a bool value. True for OK and false for NO. The reason that SET_PREPARE and DEL_PREPARE was designed as separate RPCs is because I wasn't giving too much of a thought. They could be one instead.

### DEL_PREPARE RPC
Ditto.

### COMMIT RPC
The 2nd phase of 2PC. It takes an id as its parameter, whose value determined by the coordinator.

### ABORT RPC
Ditto. 

### SET_NEXT_ID RPC
Each `set_command` and `del_command` object is assigned with an `id` by the coordinator. At the start up of the coordinator, it initializes its `next_id` field to some value that makes sense(more on this later). Each time a new client command arrives, the coordinator atomically assigns and increment this command the `next_id`. Now that we know `next_id` is monotonically incremented. The participants will try to maintain the same `next_id` as the coordinator. If they're in sync, the participant is up to date and need no recovery. If not, it's usually definitely because the participant has failed previously and the participant needs a recovery.

### NEXT_ID RPC
Used by the coordinator. It returns the `next_id` of the participants. When the coordinator comes into power, it needs a way to know whether a participant needs a recovery or not. The coordinator is always correct.

### GET_SNAPSHOT RPC
Used by the coordinator. Whenever the coordinator detects that a participant is lagging way too much. It retrieves a snapshot of the database from a correct participant. And RPC RECOVER to the out-of-date participant using the snapshot as the parameter. I know I know this is freaking crazy, but this is doable for a toy project that has a deadline!

The snapshot is simple as hell. One example is all it takes to know what's going on:

```
Suppose we have this map:

    { "blahblah": "blufff", "noise": "electric" }

Then the snapshot of this map will be like:

    ----4B---- --------8B-------- ----4B---- -------6B----- ----4B---- ----5B---- -----4B--- ------8B--------
    0x00000008 0x626C6168626C6168 0x00000006 0x626C75666666 0x00000005 6E6F697365 0x00000008 656C656374726963
    ----8----- ---blahblah------- ----6----- ----blufff---- ----5----- --noise--- -----8---- --electric------

```
Yes you got the idea. It's just a labor.

### RECOVER RPC
Once the coordinator has a snapshot, it invokes RECOVER RPC with this snapshot.

### HEARTBEAT RPC
We've chosen to use rpclib directly as the heartbeat mechanism. Heartbeat for participant failure detections.

## Two-phase commit

I read the 2PC paper way too long ago and I can barely remember anything. The 2PC seems like a simple consensus algorithm. When it comes down to implementations, there're way too many variants. Details of my choice are described below. Be warned that my implementation is probably buggy.

The coordinator relies on a logging utility: [src/record.hpp](src/record.hpp). For each update request, there can be 5 status of it. `COMMAND_UNRESOLVED`, `COMMAND_COMMIT`, `COMMAND_ABORT`, `COMMAND_COMMIT_DONE`, `COMMAND_ABORT_DONE`. 

The action of coordinator that will be taken based on events:

- On startup:
    - Read in all unresolved and unfinished records r.
    - (r.status == COMMAND_UNRESOLVED || r.status == COMMAND_ABORT) ==> Invoke ABORT RPC to all participants.
    - (r.status == COMMAND_COMMIT) ==> Invoke COMMIT RPC to all participants.

- On receiving an update cmd:
    - Assign the cmd an id and persist { cmd.id, COMMAND_UNRESOLVED, next_id } as a record to the disk.
    - Invoke PREPARE_*(cmd) RPCs to all participants.

- On receiving all PREPARE_OK:
    - Persist { cmd.id, COMMAND_COMMIT, next_id } to the disk.
    - Invoke COMMIT(cmd.id) to all participants.

- On receiving all COMMIT_OK:
    - Persist { cmd.id, COMMAND_COMMIT_DONE, next_id } to the disk.
    - Return the result to client.

- On receiving one PREPARE_FAIL:
    - Persist { cmd.id, COMMAND_ABORT, next_id } to the disk.
    - Invoke ABORT(cmd.id) to all participants.

The actions of participants:

- On PREPARE(cmd):
    - (cmd.id == next_id) ==> OK
    - otherwise ==> NO
    - next_id++

- On COMMIT(id):
    - (id < next_id - 1) ==> OK /* Seen previously. */
    - (id > next_id - 1) ==> NO /* Coordinator inconsistency. */
    - Apply the cmd with cmd.id == id
    - Return the result.

- On ABORT(id):
    - Similar approach as COMMIT.


The record format is pretty simple as well. See [src/record.cpp](src/record.cpp).