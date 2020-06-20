#include "command.hpp"

namespace cdb {

/*
GET command
*/

/// Default ctor.
get_command::get_command() : command(CMD_GET) {}

/// Convenient ctor.
get_command::get_command(std::string &&key)
    : command(CMD_GET), key_(std::move(key)) {}

/// Copy ctor.
get_command::get_command(const get_command &cmd)
    : command(cmd.type), key_(cmd.key_) {}

/// Move ctor.
get_command::get_command(get_command &&cmd)
    : get_command() { swap(*this, cmd); }

/* Assignments. */
get_command &get_command::operator=(get_command cmd)
{
    swap(*this, cmd);
    return *this;
}

get_command &get_command::operator=(get_command &&cmd)
{
    swap(*this, cmd);
    return *this;
}

/// Helper.
void swap(get_command &a, get_command &b)
{
    std::swap(a.key_, b.key_);
}

/// GETTER.
std::vector<std::string> get_command::args() const
{
    return std::vector<std::string>{key_};
}

/*
SET command.
*/

/// Default ctor
set_command::set_command()
    : command(CMD_SET) {}

/// Copy ctor
set_command::set_command(const set_command &cmd)
    : command(cmd.type), id_(cmd.id_), key_(cmd.key_), value_(cmd.value_) {}

/// Convenient ctor.
set_command::set_command(std::string &&key, std::string &&value)
    : command(CMD_SET), key_(std::move(key)), value_(std::move(value)) {}

/// Move ctor.
set_command::set_command(set_command &&cmd)
    : set_command() { swap(*this, cmd); }

/* Assignments */
set_command &set_command::operator=(set_command cmd)
{
    swap(*this, cmd);
    return *this;
}

set_command &set_command::operator=(set_command &&cmd)
{
    swap(*this, cmd);
    return *this;
}

/// GETTER.
std::vector<std::string> set_command::args() const
{
    return std::vector<std::string>{key_, value_};
}

/// Helper.
void swap(set_command &a, set_command &b)
{
    std::swap(a.id_, b.id_);
    std::swap(a.key_, b.key_);
    std::swap(a.value_, b.value_);
}

/*
DEL command.
*/

/// Default ctor.
del_command::del_command()
    : command(CMD_DEL) {}

/// Convenient ctor.
del_command::del_command(std::vector<std::string> &&keys)
    : command(CMD_DEL), keys_(std::move(keys)) {}

/// Copy ctor.
del_command::del_command(const del_command &cmd)
    : command(cmd.type), id_(cmd.id_), keys_(cmd.keys_) {}

/// Move ctor.
del_command::del_command(del_command &&cmd)
    : del_command() { swap(*this, cmd); }

/* Assignments. */
del_command &del_command::operator=(del_command cmd)
{       
    swap(*this, cmd);
    return *this;
}

del_command &del_command::operator=(del_command &&cmd)
{
    swap(*this, cmd);
    return *this;
}

/// GETTER.
std::vector<std::string> del_command::args() const
{
    return keys_;
}

/// Helper.
void swap(del_command &a, del_command &b)
{
    std::swap(a.id_, b.id_);
    std::swap(a.keys_, b.keys_);
}

}