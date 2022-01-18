#include "netqueue.h"

namespace WOF
{
    //////////////////////////////////////////////////////////////
    bool NetAddress::getSockAddr(const char* addr, uint16_t port, sockaddr_in& netaddr)
    {
        if (1 != inet_pton(AF_INET, addr, &(netaddr.sin_addr)))
        {
            return false;
        }

        netaddr.sin_family = AF_INET;
        netaddr.sin_port  = htons(port);

        return true;
    }

    //----------------------------------------------------------------------//

    //////////////////////////////////////////////////////////////

    bool NetConnection::extractSinglePackage(void* buf)
    {
        NetOpHeader* header = reinterpret_cast<NetOpHeader*>(m_buf + m_offset);
        if (m_size >= s_NetHeaderSize)
        {
            const bufsize_t package_size = header->m_size;
            if (package_size <= m_size)
            {
                memcpy(buf, m_buf + m_offset, package_size);

                m_size -= package_size;
                m_offset += package_size;

                return true;
            }
        }

        return false;
    }

    //----------------------------------------------------------------------//

    bool NetConnection::nextPacket(void* buf, bool& isclosed)
    {
        isclosed = false;

        // возможно уже есть прочитанный из сокета пакет
        if (extractSinglePackage(buf))
        {
            return true;
        }

        // подготовка буфера
        memmove(m_buf, m_buf + m_offset, m_size);
        m_offset = 0;

        // уже вычитанного из сокета пакета нет, так что читаем из сокета
        const bufsize_t readsize = s_sockSize - m_size;

        int res = recv(m_socket, m_buf + m_size, readsize, 0);
        if (0 == res)
        {
            isclosed = true;
            return false;
        }
        if (-1 == res)
        {
            const int err = errno;
            if ((-1 == res) && (EAGAIN != err) && (EWOULDBLOCK != err))
            {
                isclosed = true;
                return false;
            }
            res = 0;
        }

        m_size += res;
        if (m_size < s_NetHeaderSize)
            return false;

        return extractSinglePackage(buf);
    }

    //---------------------------------------------------------//

    bool NetConnection::sendPackets()
    {
        for (uint32_t i = 0; (i < NetQueue::s_packetsPerStep) && (!m_sendQueue.isEmpty()); ++i)
        {
            if (sockSendSize() < s_packetSize)
            {
                return true;
            }

            NetOpHeader* request = m_sendQueue.pop();
            if (nullptr == request)
            {
                return true;
            }

            const bufsize_t packet_size = request->m_size;

            assert(packet_size <= s_packetSize);

            bufsize_t nwrite = 0;
            while (nwrite < packet_size)
            {
                int res = write(m_socket, ((uint8_t*)request) + nwrite, packet_size - nwrite);
                if (res < 0)
                {
                    if (EAGAIN == errno)
                    {
                        res = 0;
                    }
                    else
                    {
                        return false;
                    }
                }

                nwrite += res;
            }

            delete[] request;
        }

        return true;
    }

    //---------------------------------------------------------//

    bufsize_t NetConnection::sockSendSize()
    {
        int option;
        socklen_t optlen = sizeof(int);
        if (0 != getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &option, &optlen))
        {
            return 0;
        }

        assert(option >= (int)s_packetSize);

        int count;
        if (0 != ioctl(m_socket, SIOCOUTQ, &count))
        {
            return 0;
        }

        const int32_t res = option - count;
        return (res > 0) ? ((bufsize_t)res / 2): 0; 
    }

    //---------------------------------------------------------//

    void NetConnection::setSockRXBuf(bufsize_t size)
    {
        const int option = (int)size;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)(&option), sizeof(int));
    }

    //---------------------------------------------------------//

    void NetConnection::setSockTXBuf(bufsize_t size)
    {
        const int option = (int)size;
        setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)(&option), sizeof(int));
    }

    //////////////////////////////////////////////////////////////

    NetQueue::NetQueue(void* object, StepCallback callback,
                        ConnectCallback connect_callback, DisconnectCallback disconnect_callback)
      : m_socket(g_badHandle),
        m_connections(),
        m_callbackObject(object),
        m_stepCallback(callback),
        m_connectCallback(connect_callback),
        m_disconnectCallback(disconnect_callback),
        m_stopFlag(false),
        m_listener(),
        m_workthread()
    { }

    //----------------------------------------------------------------------//

    NetQueue::~NetQueue()
    {
        if (g_badHandle != m_socket)
            close(m_socket);
    }

    //----------------------------------------------------------------------//
   
    bool NetQueue::init()
    {
        m_socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (g_badHandle == m_socket)
        {
            return false;
        }

        const int optval = 1;
        setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

        return true;
    }

    //---------------------------------------------------------//

    bool NetQueue::bind(const char* addr, uint16_t port)
    {
        sockaddr_in saddr;
        if (!NetAddress::getSockAddr(addr, port, saddr))
        {
            return false;
        }

        if (-1 == ::bind(m_socket, (::sockaddr*)&saddr, (socklen_t)sizeof(saddr)))
        {
            fprintf(stderr, "binderr %d", errno);
            return false;
        }

        return true;
    }

    //---------------------------------------------------------//

    NetConnection* NetQueue::connect(const char* addr, uint16_t port)
    {
        sockaddr_in saddr;
        if (!NetAddress::getSockAddr(addr, port, saddr))
        {
            return nullptr;
        }

        while (0 != ::connect(m_socket, (::sockaddr*)&saddr, (socklen_t)sizeof(saddr)))
        {
            switch (errno)
            {
            case EINTR:
                break;

            case EINPROGRESS:
            {
                pollfd handler = {.fd = m_socket, .events = POLLOUT, .revents = 0 };
                while (true){
                    const int res = poll(&handler, 1, -1);
                    if (1 == res)
                    {
                        sockaddr checkaddr;
                        socklen_t socklen = sizeof(checkaddr);
                        if (0 != getpeername(m_socket, &checkaddr, &socklen))
                        {
                            return nullptr;
                        }

                        NetConnection* conn = new NetConnection(saddr, m_socket);
                        conn->setSockTXBuf(NetConnection::s_sockSize);
                        conn->setSockRXBuf(NetConnection::s_sockSize);

                        m_connections.push(conn);

                        return conn;
                    }
                };
                
            }
            
            default:
                return nullptr;
            }
        }

        return nullptr;
    }

    //---------------------------------------------------------//

    bool NetQueue::listen()
    {
        if (0 != ::listen(m_socket, SOMAXCONN))
        {
            return false;
        }
        return true;
    }

    //---------------------------------------------------------//

    void NetQueue::start(bool init_listen)
    {
        m_stopFlag.store(false, std::memory_order_seq_cst);

        if (init_listen)
            new (&m_listener) std::thread(&NetQueue::accept, this);

        new (&m_workthread) std::thread(&NetQueue::work, this);
    }

    //---------------------------------------------------------//

    void NetQueue::stop()
    {
        m_stopFlag.store(true, std::memory_order_seq_cst);

        if (m_listener.joinable())
            m_listener.join();

        m_workthread.join();
    }

    //---------------------------------------------------------//

    void NetQueue::disconnect(NetConnection* connection)
    {
        if (nullptr != m_disconnectCallback)
        {
            m_disconnectCallback(m_callbackObject, connection);
        }
        delete connection;
    }

    //---------------------------------------------------------//

    void NetQueue::sendRequest(NetOpHeader* package, NetConnection* conn)
    {
        conn->m_sendQueue.push(package);
    }

    //---------------------------------------------------------//

    void NetQueue::accept()
    {
        while (!m_stopFlag.load(std::memory_order_relaxed))
        {
            pollfd handler;
            handler.fd = m_socket;
            handler.events = POLLIN;
            handler.revents = 0;

            const int res = poll(&handler, 1, NetConnection::s_pollTimeMs);
            if (1 == res)
            {
                sockaddr_in addr;
                socklen_t socklen = sizeof(sockaddr);
                const handle_t client_sock = ::accept(m_socket, (sockaddr*)&addr, &socklen);

                if (g_badHandle != client_sock)
                {
                    //printf("accept %d", addr.sin_addr.s_addr);

                    fcntl(client_sock, F_SETFL, O_NONBLOCK);

                    NetConnection* connect = new NetConnection(addr, client_sock);
                    connect->setSockTXBuf(NetConnection::s_sockSize);
                    connect->setSockRXBuf(NetConnection::s_sockSize);

                    if (nullptr != m_connectCallback)
                        m_connectCallback(m_callbackObject, connect);

                    m_connections.push(connect);
                }
            } 
        }
    }

    //---------------------------------------------------------//

    void NetQueue::work()
    {
        while (!m_stopFlag.load(std::memory_order_relaxed))
        {
            step();
            std::this_thread::yield();
        }
    }

    //---------------------------------------------------------//

    void NetQueue::step()
    {
        // TODO: resend

        NetConnection* connections[s_connectionsPerStep] = { 0 };
        uint32_t nconnections = 0;
        while (nconnections < s_connectionsPerStep)
        {
            NetConnection* conn = m_connections.pop();
            if (nullptr == conn)
                break;

            if (!conn->sendPackets())
            {
                fprintf(stderr, "writeerr sockerr %d\n", errno);

                // disconnect
                disconnect(conn);
                continue;
            }

            connections[nconnections] = conn;
            ++nconnections;
        }

        pollfd handlers[s_connectionsPerStep];
        for (uint32_t i = 0; i < nconnections; ++i)
        {
            handlers[i].fd = connections[i]->m_socket;
            handlers[i].events = POLLIN;
            handlers[i].revents = 0;
        }

        const int poll_res = poll(handlers, nconnections, NetConnection::s_pollTimeMs);
        if (0 < poll_res)
        {
            uint8_t* packet = new uint8_t[NetConnection::s_packetSize];

            for (uint32_t i = 0; i < nconnections; ++i)
            {
                NetConnection* const connect = connections[i];
                if (0 == (POLLIN & handlers[i].revents))
                {
                    if (0 != ((POLLERR | POLLNVAL) & handlers[i].revents))
                    {
                        fprintf(stderr, "sockerr poll %hu \n", handlers[i].revents);

                        disconnect(connect);
                        connections[i] = nullptr;
                        continue;
                    }
                }

                bool is_closed = false;
                while (connect->nextPacket(packet, is_closed))
                {
                    m_stepCallback(m_callbackObject, (NetOpHeader*)packet, connect);
                    packet = new uint8_t[NetConnection::s_packetSize];
                }

                if (is_closed)
                {
                    disconnect(connect);
                    connections[i] = nullptr;
                    continue;
                }
            }

            delete[] packet;
        }

        for (uint32_t i = 0; i < nconnections; ++i)
        {
            if (nullptr != connections[i])
                m_connections.push(connections[i]);
        }
    }

    //---------------------------------------------------------//

}