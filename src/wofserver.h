#include <random>

#include "netqueue.h"
#include "netcommands.h"

#ifndef NETSERVER_H
#define NETSERVER_H

namespace WOF
{

    //////////////////////////////////////////////////////////////////////////

    class ClientContext
    {
    public:
        ClientContext(NetConnection* conn)
          : m_nAttempts(0),
            m_word(),
            m_conn(conn)
        { }

        ~ClientContext()
        { }

        ClientContext(const ClientContext& other) = delete;
        ClientContext(ClientContext&& other) noexcept = delete;
        ClientContext& operator=(const ClientContext& other) = delete;
        ClientContext& operator=(ClientContext&& other) noexcept = delete;

    public:

        uint32_t m_nAttempts;

        std::string m_openedWord;

        std::string m_word;

        const NetConnection* m_conn;
    };

    //////////////////////////////////////////////////////////////////////////

    class NetServer
    {
    public:
        NetServer()
          : m_queue(this, NetServer::staticStepCallback, NetServer::staticConnectCallback, NetServer::staticDisconnectCallback)
        { }

        NetServer(const NetServer& other) = delete;
        NetServer(NetServer&& other) noexcept = delete;
        NetServer& operator=(const NetServer& other) = delete;
        NetServer& operator=(NetServer&& other) noexcept = delete;

        bool start(const int argc, const char* const argv[]);

        void stop();

    private:

        void Reject(NetConnection* conn);

    private:

        void ProcessRequestWord(NetOpHeader* packet, NetConnection* conn);

        void ProcessSampleChar(NetOpHeader* packet, NetConnection* conn);

    private:
        void stepCallback(NetOpHeader* request, NetConnection* conn);

        void connectCallback(NetConnection* conn);

        void disconnectCallback(NetConnection* conn);

    private:

        std::string genWord();

        static bool openLetter(ClientContext* client, const char letter);

    public:
        static inline void staticStepCallback(void* object,
                                              NetOpHeader* package,
                                              NetConnection* conn);

        static inline void staticConnectCallback(void* object,
                                              NetConnection* conn);

        static inline void staticDisconnectCallback(void* object,
                                              NetConnection* conn);

    private:

        NetQueue m_queue;

        std::vector<std::string> m_dict;

        uint32_t m_nAttempts = 2;

    public:

        static const std::string s_dictionaryPath;
    };

    //----------------------------------------------------------------------//

    void NetServer::staticStepCallback(void* object, NetOpHeader* package,
                                       NetConnection* conn)
    {
        reinterpret_cast<NetServer*>(object)->stepCallback(package, conn);
    }

    //----------------------------------------------------------------------//

    void NetServer::staticConnectCallback(void* object, NetConnection* conn)
    {
        reinterpret_cast<NetServer*>(object)->connectCallback(conn);
    }

    //----------------------------------------------------------------------//

    void NetServer::staticDisconnectCallback(void* object, NetConnection* conn)
    {
        reinterpret_cast<NetServer*>(object)->disconnectCallback(conn);
    }

    //----------------------------------------------------------------------//

    //////////////////////////////////////////////////////////////////////////

} // namespace WOF

#endif