#include <string>
#include <signal.h>
#include <iostream>
#include <condition_variable>
#include "coordinator.hpp"
#include "configuration.hpp"
#include "errors.hpp"
#include "participant.hpp"
#include "Flags.hh"
using cdb::coordinator;
using cdb::participant;
using cdb::configuration;
using cdb::coordinator_configuration;
using cdb::participant_configuration;
using cdb::configuration_manager;

/// Helper
static bool parse_addrs(std::string const &addrs, 
                        std::vector<std::string> &ips, 
                        std::vector<std::uint16_t> &ports)
{
    auto split_addr = [](std::string const &addr, std::string &ip, std::uint16_t &port)
    {
        std::stringstream ss{addr};
        if (!std::getline(ss, ip, ':'))
            return false;

        try
        {
            std::string port_str;
            if (!std::getline(ss, port_str))
                return false;
            port = std::stoi(port_str);
        } 
        catch (std::exception &e) { return false; }

        return true;
    };

    std::stringstream ss{addrs};
    std::string addr;
    while (std::getline(ss, addr, ';'))
    {
        std::string ip;
        std::uint16_t port;
        if (!split_addr(addr, ip, port))
            return false;

        ips.push_back(ip);
        ports.push_back(port);
    }

    return true;
}

int main(int argc, char *argv[])
{
    std::string mode;               /* E.g., "participant" | "coordinator" */
    std::string ip;                 /* E.g., "127.0.0.1" */
    std::uint16_t port;             /* E.g., "8001" */
    std::string config_path;        /* E.g., "./participant.conf" */

    std::string participant_addrs;  /* E.g., "127.0.0.1:8001;127.0.0.1:8002;" */
    std::string coordinator_addr;   /* E.g., "127.0.0.1:8080" */

    bool help;
    Flags flags;
    flags.Bool(help, 'h', "help", "print this message and exit");
    flags.Var(mode, 'm', "mode", std::string{"participant"}, "specify the mode of the server. [\"coordinator\" | \"participant\"]. Defaulted to \"participant\"");
    flags.Var(ip, 'a', "ip", std::string{""}, "specify an ip address. Defaulted to 127.0.0.1");
    flags.Var(config_path, 'c', "config_path", std::string{""}, "specify the path to config");
    flags.Var(port, 'p', "port", std::uint16_t{0}, "specify a port.");
    flags.Var(participant_addrs, 'P', "participant_addrs", std::string{""}, "specify a list of participant addrs separated by ';'. E.g. ip1:port1;ip2:port2");
    flags.Var(coordinator_addr, 'C', "coordinator_addr", std::string{""}, "specify the address of coordinator. E.g., 127.0.0.1:8080");

    /// TODO: parse the configuration.
    if (!flags.Parse(argc, argv))
    {
        flags.PrintHelp(argv[0]);
        abort();
    }

    if (help)
    {
        flags.PrintHelp(argv[0]);
        return 0;
    }

    std::vector<std::string> ip_addrs;
    std::vector<std::uint16_t> ports;
    std::unique_ptr<configuration> conf_ptr = nullptr;

    if (!config_path.empty())
    {
        configuration_manager conf_manager{};
        auto conf_ = conf_manager.get_conf(config_path);
        conf_ptr.reset(conf_.release());
    }

    /// TODO: Change these mess once we can merge conf.
    if (mode == "coordinator" || (conf_ptr && conf_ptr->mode == configuration::COORDINATOR))
    {
        coordinator_configuration conf;
        if (conf_ptr && conf_ptr->mode == configuration::COORDINATOR)
        {
            auto _conf_ptr = static_cast<coordinator_configuration*>(conf_ptr.get());
            conf = std::move(*_conf_ptr);
        }
        else if (conf_ptr)
            __CONF_THROW("misamatching config");

        if (!ip.empty())
            conf.addr = ip;
        if (port != 0)
            conf.port = port;

        if (conf.participant_addrs.empty() && participant_addrs.empty())
        {
            std::cerr << "coordinator was started with no participants" << std::endl;
            flags.PrintHelp(argv[0]);
            abort();
        }

        if (!participant_addrs.empty() && !parse_addrs(participant_addrs, ip_addrs, ports))
        {
            std::cerr << "invalid participant addresses" << std::endl;
            flags.PrintHelp(argv[0]);
            abort();
        }

        /// Override .
        if (!ip_addrs.empty())
        {
            conf.participant_addrs = std::move(ip_addrs);
            conf.participant_ports = std::move(ports);
        }

        std::cout << "coordinator running at [" << conf.addr << ":" << conf.port << "]\n";
        coordinator c{std::move(conf)};
        c.start();
    }
    else
    {
        participant_configuration conf;
        if (conf_ptr && conf_ptr->mode == configuration::PARTICIPANT)
        {
            auto _conf_ptr = static_cast<participant_configuration*>(conf_ptr.get());
            conf = std::move(*_conf_ptr);
        }
        else if (conf_ptr)
            __CONF_THROW("misamatching config");

        if (!ip.empty())
            conf.addr = ip;
        if (port != 0)
            conf.port = port;

        if (conf.coordinator_addr.empty() && coordinator_addr.empty())
        {
            std::cerr << "participant was started with no coordinator" << std::endl;
            flags.PrintHelp(argv[0]);
            abort();
        }

        std::vector<std::string> ip;
        std::vector<std::uint16_t> port;
        if (!coordinator_addr.empty() && !parse_addrs(coordinator_addr, ip, port))
        {
            std::cerr << "invalid coordinator address" << std::endl;
            flags.PrintHelp(argv[0]);
            abort();
        }

        /// Override.
        if (!ip.empty())
        {
            conf.coordinator_addr = ip[0];
            conf.coordinator_port = port[0];
        }

        std::cout << "participant running at [" << conf.addr << ":" << conf.port << "]\n";
        participant p{std::move(conf)};
        p.start();
    }
}