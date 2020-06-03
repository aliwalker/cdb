/// File: command.hpp
/// =================
/// Copyright 2020 Cloud-fantasy team
/// Contains command objects supported by the KV store.
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <vector>
#include "rpc/msgpack.hpp"

namespace cdb {

/// Tags.
typedef uint8_t command_type;
const command_type CMD_GET = 0;
const command_type CMD_SET = 1;
const command_type CMD_DEL = 2;
const command_type CMD_UNKNOWN = 3;

/// Base command class.
class command {
public:
    /// Ctor.
    command(command_type type = CMD_UNKNOWN) : type(type) {}
    virtual ~command() = default;

    /// Tag
    command_type type = CMD_UNKNOWN;

    /// Returns the args of this command. 
    /// NOTE: all args are serialized as std::string.
    virtual std::vector<std::string> args() const = 0;
    virtual void set_id(std::uint32_t) {}
    virtual std::uint32_t id() const { return 0; }

    // Make it serializable.
    MSGPACK_DEFINE_ARRAY(type);
};

/// GET key
class get_command : public command {
public:
    /// Ctors.
    get_command();
    get_command(std::string &&key);
    get_command(const get_command &);
    get_command(get_command &&);
    virtual ~get_command() = default;

    /// Assignment.
    get_command& operator=(get_command);
    get_command& operator=(get_command&&);

    /// GETTER.
    std::string &key() { return key_; }
    const std::string &key() const { return key_; }

    virtual std::vector<std::string> args() const override;

    MSGPACK_DEFINE_ARRAY(MSGPACK_BASE(command), key_)

private:
    friend void swap(get_command &a, get_command &b);
    /// Key into the store.
    std::string key_;
};

/// SET key value
class set_command : public command {
public:
    /// Ctors.
    set_command();
    set_command(std::string &&key, std::string &&value);
    set_command(const set_command&);
    set_command(set_command&&);
    virtual ~set_command() = default;

    /// Assignment.
    set_command& operator=(set_command);
    set_command& operator=(set_command&&);

    /// GETTER.
    virtual void set_id(std::uint32_t id) override { id_ = id; }
    virtual std::uint32_t id() const override { return id_; }

    std::string &key() { return key_; }
    const std::string &key() const { return key_; }

    std::string &value() { return value_; }
    const std::string &value() const { return value_; }

    virtual std::vector<std::string> args() const override;

    MSGPACK_DEFINE_ARRAY(MSGPACK_BASE(command), id_, key_, value_)

private:
    friend void swap(set_command &a, set_command &b);

    /// id_ is init'd by coordinator.
    std::uint32_t id_ = 0;
    std::string key_;
    std::string value_;
};

/// DEL key1 key2 ...
class del_command : public command {
public:
    /// Ctors.
    del_command();
    del_command(std::vector<std::string> &&args);
    del_command(const del_command&);
    del_command(del_command&&);
    virtual ~del_command() = default;

    /// Assignment.
    del_command& operator=(del_command);
    del_command& operator=(del_command&&);

    virtual void set_id(std::uint32_t id) override { id_ = id; }
    virtual std::uint32_t id() const override { return id_; }
    void set_keys(std::vector<std::string> const &keys) { keys_ = keys; }

    virtual std::vector<std::string> args() const override;

    MSGPACK_DEFINE_ARRAY(MSGPACK_BASE(command), id_, keys_);

private:
    friend void swap(del_command &a, del_command &b);

    /// id_ is init'd by coordinator.
    std::uint32_t id_ = 0;
    std::vector<std::string> keys_;
};

} // namespace cdb


#endif