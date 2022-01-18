
#include <assert.h>
#include <cstring>
#include <condition_variable>
#include <vector>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>

#include "types.h"

#pragma once

namespace WOF
{
    //////////////////////////////////////////////////////////////////////////

    inline bool eqArg(const char* expected, const char* arg)
    {
        return 0 == ::strcmp(expected, arg);
    }

    //////////////////////////////////////////////////////////////////////////

    class SimpleEvent
    {
    public:

        SimpleEvent()
          : m_mutex(),
            m_condvar(),
            m_sync(0)
        { }

        SimpleEvent(const SimpleEvent& other) = delete;
        SimpleEvent(SimpleEvent&& other) noexcept = delete;
        SimpleEvent& operator=(const SimpleEvent& other) = delete;
        SimpleEvent& operator=(SimpleEvent&& other) noexcept = delete;

        inline void reinit();

        inline void set();

        inline void wait();

    private:
        std::mutex m_mutex;

        std::condition_variable m_condvar;

        int32_t m_sync;
    };

    //---------------------------------------------------------//

    void SimpleEvent::reinit()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_sync = 0;
    }

    //---------------------------------------------------------//

    void SimpleEvent::set()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_sync = 1;
        m_condvar.notify_all();
    }

    //---------------------------------------------------------//

    void SimpleEvent::wait()
    {
        std::unique_lock<std::mutex> g(m_mutex);
        m_condvar.wait(g, [&] {return m_sync == 1;});
    }

    //////////////////////////////////////////////////////////////////////////

    template<class T>
    class MTRingPtr
    {
        struct Node
        {
            //Node* m_next;
            Node* m_prev;

            T* m_value;
        };

    public:

        MTRingPtr()
          : m_lock(),
            m_head(nullptr)
        { }

        ~MTRingPtr()
        {
            T* value = pop();
            while (nullptr != value)
            {
                delete value;
                value = pop();
            }
        }

        void push(T* value);

        T* pop();

        inline bool isEmpty();

    private:

        // heresy
        std::mutex m_lock;

        Node* m_head;
        Node* m_tail;
    };

    //---------------------------------------------------------//

    template<class T>
    inline bool MTRingPtr<T>::isEmpty()
    {
        std::lock_guard<std::mutex> g(m_lock);
        return (nullptr == m_head);
    }

    //---------------------------------------------------------//

    template<class T>
    void MTRingPtr<T>::push(T* value)
    {
        Node* node = new Node();
        node->m_value = value;

        std::lock_guard<std::mutex> g(m_lock);
        
        //node->m_next = m_tail;
        node->m_prev = nullptr;

        if (nullptr == m_head)
        {
            m_head = node;
        }
        else
        {
            m_tail->m_prev = node;
        }

        m_tail = node;
    }

    //---------------------------------------------------------//

    template<class T>
    T* MTRingPtr<T>::pop()
    {
        Node* node;
        {
            std::lock_guard<std::mutex> g(m_lock);
            if (nullptr == m_head)
                return nullptr;

            node = m_head;

            m_head = node->m_prev;

            if (nullptr == m_head)
            {
                m_tail = nullptr;
            }
        }

        T* res = node->m_value;
        delete node;

        return res;
    }

    //////////////////////////////////////////////////////////////////////////

    class File
    {
    public:

        static std::string cwd();

        static bool checkFileExistance(const std::string& path);

        static std::vector<std::string> getDict(const std::string& path);
    };

    //////////////////////////////////////////////////////////////////////////
        
}