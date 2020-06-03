#include <iostream>
/// File configuration.hpp
/// ======================
/// Copyright 2020 Cloud-fantasy team
/// This file contains configuration info needed by both
/// coordinator and participants.
#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace cdb {

/// Forward declaration.
struct configuration;

/// Configuration manager.
class configuration_manager {
public:
    /// Ctor.
    configuration_manager();

    /// Obtain a configuration from file [conf_path].
    std::unique_ptr<configuration> get_conf(const std::string &conf_path);

private:
    /*
    Corresponding field setters.
    */
    void coordinator_info(configuration *conf, const std::string &value);
    void participant_info(configuration *conf, const std::string &value);

    /*
    Extended fields.
    */
    void num_workers(configuration *conf, const std::string &value);
    void storage_path(configuration *conf, const std::string &value);

    bool get_option(const std::string &line, std::string &op, std::string &value);

private:
    std::unordered_map<
        std::string, 
        std::function<void(configuration*, const std::string &)>
    > m;
};

/// Configuration base class.
struct configuration {
    enum mode_t {
        COORDINATOR,
        PARTICIPANT,
        UNKNOWN
    };
    ~configuration() { std::cout << "~configuration\n"; }

    mode_t mode;
    
    /// Address used by itself.
    std::string addr;
    std::uint16_t port;

    /// Number of callback workers.
    std::size_t num_workers = 2;

    configuration() : mode(UNKNOWN) {}
    configuration(mode_t mode) : mode(mode) {}
};

/// Used by coordinator.
struct coordinator_configuration : public configuration {
    coordinator_configuration();
    coordinator_configuration(coordinator_configuration &&);
    coordinator_configuration &operator=(coordinator_configuration &&conf);

    /// IP address and ports of participants.
    std::vector<std::string> participant_addrs;
    std::vector<std::uint16_t> participant_ports;
};

/// Used by participants.
struct participant_configuration : public configuration {
    participant_configuration();
    participant_configuration(participant_configuration &&);
    participant_configuration &operator=(participant_configuration &&conf);

    /// Info of coordinator.
    std::string coordinator_addr;
    std::uint16_t coordinator_port;

    /// Path to persistent storage.
    std::string storage_path;
};

} // namespace cdb


#endif