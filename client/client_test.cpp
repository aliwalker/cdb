#include <iostream>
#include <string>
#include "cdb.hpp"
#include "tcp_server/reactor.hpp"

int main() {
    cdb::cdb_client client("127.0.0.1", 8080);
    std::string value;

    if (client.set("freak", "foo"))
        std::cout << "SET freak foo" << std::endl;

    if (client.get("freak", value))
        std::cout << "freak: " << value << std::endl;

    if (client.del("freak"))
        std::cout << "DEL freak" << std::endl;
}