#include "iostream"

#include "common.h"
#include "netqueue.h"
#include "netcommands.h"

#pragma once

namespace WOF
{
    //////////////////////////////////////////////////////////////////////////

    class ClientApp
    {
    public:
        ClientApp()
          : m_event(),
            m_queue(this, ClientApp::staticStepCallback, nullptr, nullptr),
            m_serverConnection(nullptr)
        { }

        ClientApp(const ClientApp& other) = delete;
        ClientApp(ClientApp&& other) noexcept = delete;
        ClientApp& operator=(const ClientApp& other) = delete;
        ClientApp& operator=(ClientApp&& other) noexcept = delete;

        bool init(const int argc, const char* const argv[]);

        void start();

        void deinit();

        void waitResult();

    private:

        void netRequestWord();

        void netSampleChar(const char letter);

        void stepCallback(NetOpHeader* packet, NetConnection* conn);

        void ProcessReplyWord(NetOpHeader* packet);

        void ProcessReject(NetOpHeader* packet);

    private:

        bool isWon();

    private:

        static inline void staticStepCallback(void* object,
                                              NetOpHeader* package,
                                              NetConnection* conn);

    public:

        //std::atomic<bool> m_isRejected;

        std::string m_word;

        SimpleEvent m_event;

        NetQueue m_queue;

        NetConnection* m_serverConnection;

    };

    //---------------------------------------------------------//

    void ClientApp::staticStepCallback(void* object, NetOpHeader* package,
                                       NetConnection* conn)
    {
        reinterpret_cast<ClientApp*>(object)->stepCallback(package, conn);
    }

    //---------------------------------------------------------//

} // namespace WOF