#include <iostream>
#include <sstream>
#include <fstream>
#include "configuration.hpp"
#include "errors.hpp"

namespace cdb {

/*
CTORS with default value.
*/
participant_configuration::participant_configuration()
    : configuration(PARTICIPANT)
    , coordinator_addr("127.0.0.1")
    , coordinator_port(8080)
    , storage_path("/tmp/testdb") {}

participant_configuration::participant_configuration(participant_configuration && conf)
    : configuration(PARTICIPANT)
    , coordinator_addr(std::move(conf.coordinator_addr))
    , coordinator_port(std::move(conf.coordinator_port))
    , storage_path(std::move(conf.storage_path)) {
        addr = std::move(conf.addr);
        port = conf.port;
    }

participant_configuration &participant_configuration::operator=(participant_configuration &&conf)
{
    if (this == &conf)
        return *this;

    std::swap(coordinator_addr, conf.coordinator_addr);
    std::swap(coordinator_port, conf.coordinator_port);
    std::swap(storage_path, conf.storage_path);
    std::swap(addr, conf.addr);
    std::swap(port, conf.port);
    return *this;
}

coordinator_configuration::coordinator_configuration()
    : configuration(COORDINATOR)
    , participant_addrs()
    , participant_ports() {}

coordinator_configuration::coordinator_configuration(coordinator_configuration &&conf)
    : configuration(COORDINATOR)
    , participant_addrs(std::move(conf.participant_addrs))
    , participant_ports(std::move(conf.participant_ports)) {
        addr = std::move(conf.addr);
        port = conf.port;
    }

coordinator_configuration &coordinator_configuration::operator=(coordinator_configuration &&conf)
{
    if (this == &conf)
        return *this;

    participant_addrs = std::move(conf.participant_addrs);
    participant_ports = std::move(conf.participant_ports);
    addr = std::move(conf.addr);
    port = conf.port;
    return *this;
}

bool configuration_manager::get_option(const std::string &line, std::string &op, std::string &value)
{
    int idx = 0;
    while (idx < line.size() &&
           (line[idx] != ' ' && line[idx] != '\t' && line[idx] != '\r'))
        idx++;

    if (idx == line.size())     return false;

    op = line.substr(0, idx);
    value = line.substr(idx + 1);
    return true;
}

configuration_manager::configuration_manager()
{
    m["coordinator_info"] = std::bind(&configuration_manager::coordinator_info, this, std::placeholders::_1, std::placeholders::_2);
    m["participant_info"] = std::bind(&configuration_manager::participant_info, this, std::placeholders::_1, std::placeholders::_2);
    m["num_workers"] = std::bind(&configuration_manager::num_workers, this, std::placeholders::_1, std::placeholders::_2);
    m["storage_path"] = std::bind(&configuration_manager::storage_path, this, std::placeholders::_1, std::placeholders::_2);
}

std::unique_ptr<configuration>
configuration_manager::get_conf(const std::string &conf_path)
{
    std::ifstream conf_str{conf_path};
    if (!conf_str.is_open())
        __SERVER_THROW("configuration file does not exist");

    std::unique_ptr<configuration> ret = nullptr;
    std::string line;
    bool mode_set = false;
    while (std::getline(conf_str, line))
    {
        /// Ignore comment lines.
        if (line[0] == '!' || line.empty()) continue;

        std::string op, value;
        if (!get_option(line, op, value))
            __SERVER_THROW("invalid configuration file");

        if (!mode_set && op == "mode" && value == "coordinator")
        {
            ret.reset(new coordinator_configuration);
            mode_set = true;
            continue;
        }
        
        if (!mode_set && op == "mode" && value == "participant")
        {
            ret.reset(new participant_configuration);
            mode_set = true;
            continue;
        }

        if (!mode_set)
            __CONF_THROW("first field is not mode");

        if (!m.count(op))
            __CONF_THROW("unknown configuration field");

        (m[op])(ret.get(), value);
    }
    return ret;
}

void
configuration_manager::coordinator_info(configuration *conf, const std::string &value)
{
    auto *coor_conf = static_cast<coordinator_configuration*>(conf);
    auto *part_conf = static_cast<participant_configuration*>(conf);

    std::stringstream ss(value);
    std::string ip;
    std::string port;
    if (!std::getline(ss, ip, ':'))
        __CONF_THROW("invalid coorinator_info");

    if (!std::getline(ss, port))
        __CONF_THROW("invalid coordinator_info");

    try
    {
        if (conf->mode == configuration::COORDINATOR)
        {
            coor_conf->addr = ip;
            coor_conf->port = std::stoi(port);
        }

        if (conf->mode == configuration::PARTICIPANT)
        {
            part_conf->coordinator_addr = ip;
            part_conf->coordinator_port = std::stoi(port);
        }
    }
    catch (std::exception &e) { __CONF_THROW("invalid coordinator_info"); }
}

void
configuration_manager::participant_info(configuration *conf, const std::string &value)
{
    auto *coor_conf = static_cast<coordinator_configuration*>(conf);
    auto *part_conf = static_cast<participant_configuration*>(conf);

    std::stringstream ss(value);
    std::string ip;
    std::string port;
    if (!std::getline(ss, ip, ':'))
        __CONF_THROW("invalid coorinator_info");

    if (!std::getline(ss, port))
        __CONF_THROW("invalid coordinator_info");

    try
    {
        if (conf->mode == configuration::COORDINATOR)
        {
            coor_conf->participant_addrs.push_back(ip);
            coor_conf->participant_ports.push_back(std::stoi(port));
        }

        if (conf->mode == configuration::PARTICIPANT)
        {
            part_conf->addr = ip;
            part_conf->port = std::stoi(port);
        }
    } catch (std::exception &e) { __CONF_THROW("invalid participant_info"); }
}

void 
configuration_manager::num_workers(configuration *conf, const std::string &value)
{
    try
    {
        std::size_t num_workers = std::stoul(value);
        conf->num_workers = num_workers;
    } catch (std::exception &e) { __CONF_THROW("invalid number of workers"); }
}

void
configuration_manager::storage_path(configuration *conf, const std::string &value)
{
    if (conf->mode == configuration::COORDINATOR)
        __CONF_THROW("storage path specified in coordinator configuration");

    static_cast<participant_configuration*>(conf)->storage_path = value;
}

}   // namespace cdb