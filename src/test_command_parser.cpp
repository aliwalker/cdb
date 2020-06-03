#include <string>
#include <iostream>
#include "command_parser.hpp"
using namespace cdb;

template <class command_t>
void parse(std::string const &cmd_str)
{
    command_parser parser{std::vector<char>{cmd_str.begin(), cmd_str.end()}};

    try {
        std::unique_ptr<command_t> cmd_obj{ dynamic_cast<command_t*>(parser.read_command().release()) };
        std::vector<std::string> args = cmd_obj->args();
        
        switch (cmd_obj->type)
        {
            case CMD_GET: std::cout << "GET "; break;
            case CMD_SET: std::cout << "SET "; break;
            case CMD_DEL: std::cout << "DEL "; break;
            default: break;
        }

        for (auto &arg : args)
            std::cout << arg << " ";
        std::cout << std::endl;
    } catch (std::runtime_error &e) {
        std::cout << "Caught error: " << e.what() << std::endl;
    }
}

int main()
{
    std::string del_cmd{"*3\r\n$3\r\nDEL\r\n$7\r\nCS06142\r\n$5\r\nCS162\r\n"};
    std::string get_cmd{"*2\r\n$3\r\nGET\r\n$7\r\nCS06142\r\n"};
    std::string set_cmd{"*4\r\n$3\r\nSET\r\n$7\r\nCS06142\r\n$5\r\nCloud\r\n$9\r\nComputing\r\n"};

    parse<del_command>(del_cmd);
    parse<get_command>(get_cmd);
    parse<set_command>(set_cmd);
}