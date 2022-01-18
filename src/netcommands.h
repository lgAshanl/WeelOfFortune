#include <cstring>

#include "netqueue.h"

#pragma once

namespace WOF
{
    //////////////////////////////////////////////////////////////////////////

    /*
     * Структуры, а не функции для наполнения пакетов командой потому,
     * что так нагляднее: по полям видно содержание команды и последовательность  
     */

    //////////////////////////////////////////////////////////////////////////

    class CommandCommon
    {
    public:
        CommandCommon(const CommandCommon& other) = delete;
        CommandCommon(CommandCommon&& other) noexcept = delete;
        CommandCommon& operator=(const CommandCommon& other) = delete;
        CommandCommon& operator=(CommandCommon&& other) noexcept = delete;

    protected:
        CommandCommon() = default;
    };

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////

    class CommandRequestWord : public CommandCommon
    {
    public:
        CommandRequestWord()
        { };

        void fillRequest(NetOpHeader* header)
        {
            header->m_type = NetRequestType::RequestWord;
            header->m_size = sizeof(NetOpHeader);
        }
    };

    //////////////////////////////////////////////////////////////////////////

    class CommandReplyWord : public CommandCommon
    {
    public:
        CommandReplyWord(const std::string& word)
          : m_word(word)
        { };

        void fillRequest(NetOpHeader* header)
        {
            uint8_t* packet = (uint8_t*)header;
            header->m_type = NetRequestType::ReplyWord;
            size_t offset = sizeof(NetOpHeader);

            memcpy(packet + offset, m_word.c_str(), m_word.size() + 1);
            offset += m_word.size() + 1;

            const size_t packet_size = offset;
            header->m_size = packet_size;
        }

    public:

        const std::string& m_word;
    };

    //////////////////////////////////////////////////////////////////////////

    class CommandSampleChar : public CommandCommon
    {
    public:
        CommandSampleChar(const char letter)
          : m_letter(letter)
        { };

        void fillRequest(NetOpHeader* header)
        {
            uint8_t* packet = (uint8_t*)header;
            header->m_type = NetRequestType::SampleChar;
            size_t offset = sizeof(NetOpHeader);

            memcpy(packet + offset, &m_letter, sizeof(m_letter));
            offset += sizeof(m_letter);

            const size_t packet_size = offset;
            header->m_size = packet_size;
        }

    public:

        const char m_letter;
    };

    //////////////////////////////////////////////////////////////////////////

    class CommandReject : public CommandCommon
    {
    public:
        CommandReject()
        { };

        void fillRequest(NetOpHeader* header)
        {
            header->m_type = NetRequestType::Reject;
            header->m_size = sizeof(NetOpHeader);
        }
    };


}