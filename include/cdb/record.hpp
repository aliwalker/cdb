/// File record.hpp
/// ===============
/// This file contains the record and record_manager used by the coordinator.
/// The coordinator relys on logged record to recover.
#ifndef CDB_RECORD_HPP
#define CDB_RECORD_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <memory>
#include "command.hpp"

namespace cdb {

typedef std::uint8_t record_status_t;

/*
Coordinator record status.
*/
/// UNRESOLVED means the coordinator has not yet made a decision.
const record_status_t RECORD_UNRESOLVED = 0;

/*
Common record status.
*/
/// COMMIT means the coordinator has made the decision to commit.
const record_status_t RECORD_COMMIT = 1;
/// ABORT means the coordinator has made the decision to abort.
const record_status_t RECORD_ABORT = 2;
/// COMMIT_DONE means the coordinator has ensured the request has been committed to
/// all participants(specifically, current set of participants)
const record_status_t RECORD_COMMIT_DONE = 3;
/// ABORT_DONE has similar meaning as COMMIT_DONE.
const record_status_t RECORD_ABORT_DONE = 4;

/*
Participant record status.
*/
const record_status_t RECORD_PREPARED = 5;

/// A record represents a client request/command that needs 2PC.
/// The record is persisted on disk.
struct record {
public:
    /// Ctor.
    record() = default;
    record(record_status_t s, std::uint32_t id, std::uint32_t next_id)
        : status(s), id(id), next_id(next_id) {}

    record_status_t status = RECORD_UNRESOLVED;

    /// ID of the cmd.
    std::uint32_t id;

    /// Used upon coordinator recovery.
    std::uint32_t next_id;

    /// Convert to binary.
    std::vector<unsigned char> to_binary() const;

    /// Parse binary to a record.
    static record parse(const std::vector<unsigned char> &binary);

    /// The size of a single record.
    static const std::uint32_t record_size = 9;

private:
    friend class record_manager;
    static void encode_uint32_t(std::vector<unsigned char> &binary, std::uint32_t val);
    static void encode_uint8_t(std::vector<unsigned char> &binary, std::uint8_t val);

    static std::uint32_t decode_uint32_t(const std::vector<unsigned char> &binary, std::size_t &idx);
    static std::uint8_t decode_uint8_t(const std::vector<unsigned char> &binary, std::size_t &idx);
};

/// Record_manager used by the coordinator to persist update request info.
/// NOTE: this class is not thread-safe. Use external locking to prevent data races.
class record_manager {
public:
    /// [file_name] is the name of the log.
    record_manager(std::string const &file_name);

    /// Log a record.
    void log(const record &r);

    /// Log a command. Used by the participant.
    void log(const command *cmd);

    std::uint32_t next_id() const { return next_id_; }

    /// GETTER.
    std::map<std::uint32_t, record> &records() { return records_; }
    std::map<std::uint32_t, std::unique_ptr<command>> &cmds() { return cmds_; }

private:
    /// Initialize [records_]. Called within ctor.
    void init_records();

private:
    /// Name of the log file.
    std::string file_name_;

    /// Actual log file.
    std::fstream file_;
    std::fstream cmd_file_;

    /// The up-to-date next_id to be set as the coordinator's next_id_.
    std::uint32_t next_id_ = 0;

    /// In-memory records.
    std::map<std::uint32_t/* ID */, record> records_;
    std::map<std::uint32_t/* ID */, std::unique_ptr<command>> cmds_;
};

} // namespace cdb


#endif