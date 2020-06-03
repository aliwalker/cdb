#include <iostream>
#include "../third_party/flags.hh/Flags.hh"
#include "record.hpp"

void log(cdb::record_manager &manager, std::uint8_t s, std::uint32_t id, std::uint32_t next_id)
{
    cdb::record r{s, id, next_id};
    manager.log(r);
}

void parse(cdb::record_manager &manager)
{
    auto records = manager.records();

    for (auto &r : records)
    {
        std::cout << "record: " << (int)r.second.status << " " << r.second.id << " " << r.second.next_id << std::endl;
    }
}

int main(int argc, char **argv)
{
    cdb::record_manager manager("coordinator.log");

    bool _log, _parse;
    Flags flags;

    int status;
    std::uint32_t id, next_id;

    flags.Bool(_log, 'l', "log", "commit a log");
    flags.Bool(_parse, 'p', "parse", "commit a parse");
    flags.Var(status, 's', "status", int{0}, "status of the log");
    flags.Var(id, 'i', "id", std::uint32_t{0}, "id of the log");
    flags.Var(next_id, 'I', "next-id", std::uint32_t{1}, "next id");

    if (!flags.Parse(argc, argv))
    {
        flags.PrintHelp(argv[0]);
        exit(-1);
    }

    if (_log)
        log(manager, status, id, next_id);
    
    else if (_parse)
        parse(manager);

    return 0;
}