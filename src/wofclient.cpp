#include "wofclient.h"
#include <stdio.h>

namespace WOF
{
    //////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////

    bool ClientApp::init(const int argc, const char* const argv[])
    {

        std::string serveraddr;

        for (int32_t i = 1; i < argc; ++i)
        {
            if (eqArg("-server", argv[i]))
            {
                if ((i + 1) < argc)
                    serveraddr = std::string(argv[i+1]);
                else
                {
                    printf("-server arg skiped\n");
                    printf("Usage: -server 69.69.69.69\n");
                    return false;
                }
                ++i;
            }
            else
            {
                printf("Unsupported arg %s\n", argv[i]);
                printf("Usage: -server 69.69.69.69\n");
                return false;
            }
        }

        // check args
        if (serveraddr.empty())
        {
            printf("-server address arg skiped, try 127.0.0.1\n");
            serveraddr = "127.0.0.1";
        }

        // realinit

        if (!m_queue.init())
        {
            printf("CLI init queue err %d\n", errno);
            return false;
        }

        m_serverConnection = m_queue.connect(serveraddr.c_str(), 30666);
        if (nullptr == m_serverConnection)
        {
            printf("Connect To Server Failed\n");
            return false;
        }
        assert(nullptr != m_serverConnection);

        return true;
    }

    //---------------------------------------------------------//

    void ClientApp::start()
    {
        m_queue.start(false);

        // work        
        netRequestWord();
        waitResult();

        if (m_word.empty())
        {
            printf("word recieve failed\n");
            return;
        }

        std::string command;
        while (true)
        {
            std::cin >> command;

            if (eqArg("quit", command.c_str()))
            {
                return;
            }

            if (command.size() != 1)
            {
                printf("Please enter single character\n");
                continue;
            }

            netSampleChar(command[0]);
            waitResult();

            if (m_word.empty())
            {
                std::cout << "You lost attempts\n";
                return;
            }

            if (isWon())
            {
                std::cout << "You won!\n";;
                return;
            }
        }
        
    }

    //---------------------------------------------------------//

    void ClientApp::deinit()
    {
        m_queue.stop();
    }

    //---------------------------------------------------------//

    void ClientApp::waitResult()
    {
        m_event.wait();
    }

    //---------------------------------------------------------//

    void ClientApp::netRequestWord()
    {
        m_event.reinit();

        NetOpHeader* request = m_queue.allocPackage();

        CommandRequestWord command;
        command.fillRequest(request);

        m_queue.sendRequest(request, m_serverConnection);
    }

    //---------------------------------------------------------//

    void ClientApp::netSampleChar(const char letter)
    {
        m_event.reinit();

        NetOpHeader* request = m_queue.allocPackage();

        CommandSampleChar command(letter);
        command.fillRequest(request);

        m_queue.sendRequest(request, m_serverConnection);
    }

    //---------------------------------------------------------//

    void ClientApp::stepCallback(NetOpHeader* packet, NetConnection* conn)
    {
        (void)conn;
        switch (packet->m_type)
        {
        case NetRequestType::RequestWord:
            assert(false);
            break;

        case NetRequestType::ReplyWord:
            ProcessReplyWord(packet);
            break;

        case NetRequestType::SampleChar:
            assert(false);
            break;

        case NetRequestType::Reject:
            ProcessReject(packet);
            break;

        default:
            assert(false);

            break;
        }

        delete[] packet;
    }

    //---------------------------------------------------------//

    void ClientApp::ProcessReplyWord(NetOpHeader* packet)
    {
        m_word = std::string(((char*)packet) + sizeof(NetOpHeader));

        printf("word: %s\n", m_word.c_str());

        m_event.set();
    }

    //---------------------------------------------------------//

    void ClientApp::ProcessReject(NetOpHeader* packet)
    {
        (void)packet;
        m_word.clear();
        m_event.set();
    }

    //---------------------------------------------------------//

    bool ClientApp::isWon()
    {
        return std::string::npos == m_word.find('*');
    }

    //////////////////////////////////////////////////////////////

    
    //---------------------------------------------------------//

} // namespace WOF