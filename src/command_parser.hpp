/// File command_parser.hpp
/// =======================
/// Copyright 2020 Cloud-fantasy team
/// The parser is used by the coordinator.
#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

#include <string>
#include <vector>
#include "command.hpp"

namespace cdb {

/// Command parser parses a command from the given data.
/// If the given data is not complete, the parser simply rejects
/// and throws a parse_error.
/// We've designed the parser to be synchronous. This simplified the whole
/// coordinator design.
class command_parser {
public:
    /*
    Move sementics.
    */
    /// Ctor.
    command_parser() = default;
    command_parser(const command_parser&) = delete;
    command_parser(command_parser &&);
    ~command_parser() = default;

    /// Convenient ctor.
    command_parser(std::vector<char> &&data);

    /// Assignment.
    command_parser& operator=(const command_parser&) = delete;
    command_parser& operator=(command_parser &&);

    /// GETTER
    std::vector<char> &data() { return data_; }
    const std::vector<char> &data() const { return data_; }

    /// Read a command from the given data.
    std::unique_ptr<command> read_command();

    /// Indicating whether parser is done.
    bool is_done();

    /// How many bytes are left to be parsed.
    std::size_t bytes_parsed();

public:
    /// "\r\n" as default separator.
    static std::string separator;

private:
    /// Read a single charactor.
    char read_char();
    /// Unread a single charactor.
    char peek_char();

    /// Read a RESP bulk string.
    std::string read_bulk_string();

    /// Read the number of elems in the current RESP array.
    std::size_t read_num_elem();

    /// Type of the command.
    command_type read_command_type();

    /// Specific commands.
    std::unique_ptr<command> read_get_command(std::size_t num_elem);
    std::unique_ptr<command> read_set_command(std::size_t num_elem);
    std::unique_ptr<command> read_del_command(std::size_t num_elem);

    void expect_char(char ch, const std::string &msg = "mismatching char");
    void expect_separator();

private:
    /// Buffer for data from [client_].
    std::vector<char> data_;
    std::size_t idx_;

    /// Starting index of a command.
    std::size_t start_idx_;
};

}    // namespace cdb

#endif