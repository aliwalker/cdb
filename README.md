<p align="center">
    <img src="./logo.png">
</p>

# Cdb

Cdb is simple distributed key-value data store that utilizes 2-phase commit and leveldb. It consists of one coordinator and multiple participants. The client interacts with the coordinator using a simplified RESP message format over a TCP connection, while the coordinator sends RPC to all its participants.

Cdb implements the requirements of the [Extreme version](https://github.com/1989chenguo/CloudComputingLabs/tree/master/Lab3#353-extreme-version) of the [CloudComputingLabs](https://github.com/1989chenguo/CloudComputingLabs/tree/master/Lab3). This means that the following claims hold:

- The system supports only `GET`, `SET` and `DEL` commands. All commands are sent in a simplied RESP format to coordinator.
- The system can function correctly(without inconsistency) as long as both the following conditions hold:
    - the coordinator is up and running and
    - at least one participant is *working correctly*. 
    
    By working correctly, we mean that the coordinator is as up-to-date as the coordinator and is capable of communicating with the coordinator. If however, one of the above two conditions does not hold, it requires intervention to satisfy them. In most cases, where there's no physical damage, all you have to do is to restart the dead servers.
- The system can tolerate the coordinator failures. If the coordinator fails when it's idle, nothing fancy happens. However, if the coordinator fails during a 2PC update, there're 2 scenarios needed to pay attention to:
    - if the coordinator has resolved the client request, i.e., either to commit or abort, then the corresponding action will be taken place when the coordinator comes online again. 
    - if the coordinator has not resolved the client request, i.e., the coordinator hasn't received all `PREPARE_OK` messages(from its current set of participants at that moment), then abort will be taken place when the coordinator comes online again.
- `SET` and `DEL` will go through 2PC, while `GET` will not. This means if there are concurrent clients updating the database, a client may get a stale value. However, the FIFO client order is guaranteed since we're using TCP for receiving commands from clients.
- The clients get replies only when the coordinator is up and running. 
- Bonus: since we're utilizing leveldb, you can actually persist your data!

For more details, please look into the [internal_documentation.md](./internal_documentation.md).

## Requirments

- Linux/macOS.
- GNU make.
- C++11 compliant compiler.

Note that we only support Linux and macOS. Other *nix OS probably does not compile the source. 
Also note, to simplify your target machine setup, we've stripped all CMake related build-system and instead adding some ad-hoc Makefiles such that you can compile the whole project with a simple make command:

```
make
```

## Usage

```
./kvstore2pcsystem [options]

  -h --help [default: (unset)]
      print this message and exit
  -m --mode [default: participant]
      specify the mode of the server. ["coordinator" | "participant"]. Defaulted
       to "participant"
  -a --ip [default: ]
      specify an ip address. Defaulted to 127.0.0.1
  -c --config_path [default: ]
      specify the path to config
  -p --port [default: 0]
      specify a port.
  -P --participant_addrs [default: ]
      specify a list of participant addrs separated by ';'. E.g. "ip1:port1;ip2:p"
      ort2
  -C --coordinator_addr [default: ]
      specify the address of coordinator. E.g., 127.0.0.1:8080
```

Apart from the requirements of the Extreme version states, we've added several convenient options to run the server. The coordinator and participants can be started in any orders. 

To use a configuaration, see [src/coordinator.conf](src/coordinator.conf) and [src/participant.conf](src/participant.conf) for sample coordinator and participant configuarations, repectively.

## TODOS
There're a lot of improvements to make. However, since I'm the only developer of this DB, I do not have enought time.

- Add a logger.
- Write a frontend/cli.
- Remove all the crazy Makefile things.
- Make coordinator fault-tolerant.
