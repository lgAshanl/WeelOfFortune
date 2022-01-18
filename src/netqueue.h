#include <string>
#include <atomic>
#include <thread>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/sockios.h>
#include <fcntl.h>

#include "types.h"
#include "common.h"

#pragma once

namespace WOF
{
    // FD:
    class NetConnection;

    //////////////////////////////////////////////////////////////////////////

    enum NetRequestType
    {
        RequestWord = 1,
        ReplyWord = 2,
        SampleChar = 3,
        Reject = 9
    };

    //////////////////////////////////////////////////////////////////////////

    struct NetOpHeader
    {
        NetRequestType m_type;

        bufsize_t m_size;
    };    

    //////////////////////////////////////////////////////////////////////////

    class NetAddress
    {
    public:

        explicit NetAddress(sockaddr_in sock)
          : m_addr(sock)
        { };

        static bool getSockAddr(const char* addr, uint16_t port, sockaddr_in& netaddr);

    public:

        const struct sockaddr_in m_addr;
    };

    //////////////////////////////////////////////////////////////////////////

    class NetConnection
    {
        friend class NetQueue;
    public:
        NetConnection(sockaddr_in addr, handle_t socket)
          : m_addr(addr),
            m_socket(socket),
            m_context(nullptr),
            m_buf(new uint8_t[s_sockSize]),
            m_size(0)
        { };

        ~NetConnection()
        {
            if (g_badHandle != m_socket)
                close(m_socket);
            delete[] m_buf;
        }

        inline void setContext(void* context);

    private:

        bool extractSinglePackage(void* buf);

        bool nextPacket(void* buf, bool& isclosed);

        bool sendPackets();

        bufsize_t sockSendSize();

        void setSockRXBuf(bufsize_t size);

        void setSockTXBuf(bufsize_t size);

    public:

        static constexpr bufsize_t s_NetHeaderSize = sizeof(NetOpHeader);
    
        static constexpr bufsize_t s_packetSize = UINT16_MAX;

        //static constexpr bufsize_t s_sockSize = 2 << 17;
        static constexpr bufsize_t s_sockSize = 2 << 17;

        static constexpr bufsize_t s_pollTimeMs = 1;

    public:

        const NetAddress m_addr;

        const handle_t m_socket;

        void* m_context;

    private:

        MTRingPtr<NetOpHeader> m_sendQueue;

        uint8_t* m_buf;

        bufsize_t m_offset;

        bufsize_t m_size;
    };

    //---------------------------------------------------------//

    void NetConnection::setContext(void* context)
    {
        m_context = context;
    }

    //////////////////////////////////////////////////////////////////////////

    class NetQueue
    {
    public:

        typedef void (*StepCallback)(void* object, NetOpHeader* package, NetConnection* conn);

        typedef void (*ConnectCallback)(void* object, NetConnection* conn);

        typedef void (*DisconnectCallback)(void* object, NetConnection* conn);

        NetQueue() = delete;

        NetQueue(void* object, StepCallback callback,
                ConnectCallback connect_callback, DisconnectCallback disonnect_callback);

        ~NetQueue();

        bool init();

        bool bind(const char* addr, uint16_t port);

        NetConnection* connect(const char* addr, uint16_t port);

        bool listen();

        void start(bool init_listen = false);

        void stop();

        void disconnect(NetConnection* connection);

    public:

        void sendRequest(NetOpHeader* package, NetConnection* conn);

        inline NetOpHeader* allocPackage();

        inline void freePackage(NetOpHeader* request);

    private:  

        void accept();

        void work();

        void step();

    private:

        handle_t m_socket;

        MTRingPtr<NetConnection> m_connections;

        void* m_callbackObject;

        StepCallback m_stepCallback;

        ConnectCallback m_connectCallback;

        DisconnectCallback m_disconnectCallback;

        std::atomic<bool> m_stopFlag;

        std::thread m_listener;

        std::thread m_workthread;

    public:

        static constexpr uint32_t s_connectionsPerStep = 64;

        static constexpr uint32_t s_packetsPerStep = 8;
    };

    //----------------------------------------------------------------------//

    inline NetOpHeader* NetQueue::allocPackage()
    {
        uint8_t* package = new uint8_t[NetConnection::s_packetSize];
        return (NetOpHeader*)package;
    }

    //----------------------------------------------------------------------//

    inline void NetQueue::freePackage(NetOpHeader* package)
    {
        return delete[] package;
    }

    //----------------------------------------------------------------------//

    //////////////////////////////////////////////////////////////////////////

}