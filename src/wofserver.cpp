#include "wofserver.h"

namespace WOF
{
    //////////////////////////////////////////////////////////////

    const std::string NetServer::s_dictionaryPath(File::cwd() + "/dictionary.txt");

    //---------------------------------------------------------//

    bool NetServer::start(const int argc, const char* const argv[])
    {
        std::string nattempts;
        for (int32_t i = 1; i < argc; ++i)
        {
            if (eqArg("-nattempts", argv[i]))
            {
                if ((i + 1) < argc)
                {
                    nattempts = std::string(argv[i+1]);
                    bool err = false;
                    int num;
                    try
                    {
                        num = std::stoi(nattempts);
                        err = (num <= 0);
                    }
                    catch(...)
                    {
                        err = true;
                    }
                    
                    if (err)
                    {
                        printf("invalid attempts number\n");
                        return false;
                    }

                    m_nAttempts = (uint32_t)num;
                }
                else
                {
                    printf("-nattempts arg skiped\n");
                    printf("Usage: -nattempts 69\n");
                    return false;
                }
                ++i;
            }
            else
            {
                printf("Unsupported arg %s\n", argv[i]);
                printf("Usage: -nattempts 69\n");
                return false;
            }
        }

        // realinit
        if (!m_queue.init())
        {
            fprintf(stderr, "init queue err %d", errno);
            return false;
        }
        
        //if (!m_queue.bind("127.0.0.1", 30666))
        if (!m_queue.bind("0.0.0.0", 30666))
        {
            fprintf(stderr, "bind queue err %d", errno);
            return false;
        }

        if (!File::checkFileExistance(s_dictionaryPath))
        {
            printf("no dict file\n");
            return false;
        }

        m_dict = File::getDict(s_dictionaryPath);

        if (!m_queue.listen())
        {
            printf("init queue err %d", errno);
            return false;
        }

        m_queue.start(true);

        return true;
    }

    //---------------------------------------------------------//

    void NetServer::stop()
    {
        m_queue.stop();
    }

    //---------------------------------------------------------//

    void NetServer::stepCallback(NetOpHeader* packet, NetConnection* conn)
    {
        switch (packet->m_type)
        {
        case NetRequestType::RequestWord:
            ProcessRequestWord(packet, conn);
            break;

        case NetRequestType::ReplyWord:
            assert(false);
            break;

        case NetRequestType::SampleChar:
            ProcessSampleChar(packet, conn);
            break;

        case NetRequestType::Reject:
            assert(false);
            break;

        default:
            assert(false);

            break;
        }

        delete[] packet;
    }

    //---------------------------------------------------------//

    void NetServer::connectCallback(NetConnection* conn)
    {
        conn->setContext(new ClientContext(conn));
    }

    //---------------------------------------------------------//

    void NetServer::disconnectCallback(NetConnection* conn)
    {
#if !NDEBUG
        fprintf(stderr, "client disconnected\n");
#endif
        delete ((ClientContext*)conn->m_context);
        conn->setContext(nullptr);
    }

    //---------------------------------------------------------//

    std::string NetServer::genWord()
    {
        if (m_dict.empty())
            return std::string();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(0, m_dict.size() - 1);
        return m_dict[distrib(gen)];
    }

    //---------------------------------------------------------//

    bool NetServer::openLetter(ClientContext* client, const char letter)
    {
        bool suc_flag = false;
        const size_t size = client->m_word.size();
        for (uint32_t i = 0; i < size; ++i)
        {
            if (letter == client->m_word[i])
            {
                client->m_openedWord[i] = letter;
                suc_flag = true;
            }
        }

        return suc_flag;
    }

    //---------------------------------------------------------//

    void NetServer::Reject(NetConnection* conn)
    {
        NetOpHeader* request = m_queue.allocPackage();

        CommandReject command;
        command.fillRequest(request);

        m_queue.sendRequest(request, conn);
    }

    //---------------------------------------------------------//

    void NetServer::ProcessRequestWord(NetOpHeader* packet, NetConnection* conn)
    {
        (void)packet;
        ClientContext* context = (ClientContext*)conn->m_context;
        assert(nullptr != context);

        context->m_nAttempts = m_nAttempts;
        context->m_word = genWord();
        context->m_openedWord = std::string(context->m_word.size(), '*');

        NetOpHeader* request = m_queue.allocPackage();

        CommandReplyWord command(context->m_openedWord);
        command.fillRequest(request);

        m_queue.sendRequest(request, conn);
    }

    //---------------------------------------------------------//

    void NetServer::ProcessSampleChar(NetOpHeader* packet, NetConnection* conn)
    {
        const char letter = *((char*)packet + sizeof(NetOpHeader));

        ClientContext* context = (ClientContext*)conn->m_context;
        assert(nullptr != context);

        if (0 == context->m_nAttempts)
        {
            Reject(conn);
            return;
        }

        if (!NetServer::openLetter(context, letter))
        {
            --context->m_nAttempts;
            if (0 == context->m_nAttempts)
            {
                Reject(conn);
                return;
            }
        }

        NetOpHeader* request = m_queue.allocPackage();

        CommandReplyWord command(context->m_openedWord);
        command.fillRequest(request);

        m_queue.sendRequest(request, conn);
    }

    //---------------------------------------------------------//

}