#include "wofserver.h"
#include "iostream"

int main(int argc, char* argv[])
{
    WOF::NetServer server;
    if (!server.start(argc, argv))
    {
        std::cout << "server start failed\n";
        return 1;
    }

    std::string line;
    while (true)
    {
        std::cin >> line;
        if ("stop" == line)
        {
            break;
        }
    }

    server.stop();

    return 0;
}