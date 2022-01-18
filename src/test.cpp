#include <gtest/gtest.h>
#include <iostream>
#include <streambuf>

#include "wofclient.h"
#include "wofserver.h"
#include "stdint.h"

class TestClientServer : public ::testing::Test
{
protected:
	void SetUp()
	{
        std::string fakefile = WOF::File::cwd() + "/fakefile";
        m_handle = ::open(fakefile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
        ASSERT_NE(-1, m_handle);

        int pfd[2] = {-1,-1};
        pipe(pfd);
        dup2(pfd[0],STDIN_FILENO);
        dup2(pfd[1],m_handle);

        std::cout.rdbuf(m_out.rdbuf());
	}

	void TearDown()
	{
		close(m_handle);
        close(STDIN_FILENO);
        emptyInput();
	}

    static void clientWork(WOF::ClientApp* client)
    {
        client->start();
    }

    void probeLetter(char letter)
    {
        char str[3] = {letter, '\r'};
        write(m_handle, str, 2);
        fsync(m_handle);
    }

    void assertGoodResult()
    {
        std::string output = m_out.str();
        m_out.clear();

        const size_t size = output.size();
        const std::string last_word = output.substr(size - 9, size - 1);
        ASSERT_EQ(last_word, "You won!");
    }

    void assertAttempts()
    {
        std::string output = m_out.str();
        m_out.clear();

        const size_t size = output.size();
        const std::string last_word = output.substr(size - 18, size - 1);
        ASSERT_EQ(last_word, "You lost attempts");
    }

    void emptyInput()
    {
        //std::string line;

        //do
        //{
        //    getline(std::cin, line);
        //} while (!line.empty());
        std::cin.clear();
    }

protected:

    int m_handle;

    std::stringstream m_out;
};

TEST_F(TestClientServer, simple)
{
    WOF::NetServer server;
    const int serv_argc = 3;
    const char* serv_argv[3] = {"test", "-nattempts", "20"};
    ASSERT_TRUE(server.start(serv_argc, serv_argv));

    const int argc = 3;
    const char* argv[3] = {"test", "-server", "127.0.0.1"};

    {
        WOF::ClientApp client;
        ASSERT_TRUE(client.init(argc, argv));

        std::thread client_thread(&clientWork, &client);

        probeLetter('I');
        probeLetter('N');
        probeLetter('e');
        probeLetter('v');
        probeLetter('e');
        probeLetter('r');
        probeLetter('A');
        probeLetter('s');
        probeLetter('k');
        probeLetter('e');
        probeLetter('d');
        probeLetter('F');
        probeLetter('o');
        probeLetter('r');
        probeLetter('T');
        probeLetter('h');
        probeLetter('i');
        probeLetter('s');

        client_thread.join();

        assertGoodResult();

        client.deinit();
    }

    sleep(1);

    server.stop();
}

TEST_F(TestClientServer, attempts)
{
    WOF::NetServer server;
    const int serv_argc = 1;
    const char* serv_argv[1] = {"test"};
    ASSERT_TRUE(server.start(serv_argc, serv_argv));

    const int argc = 1;
    const char* argv[1] = {"test"};

    {
        WOF::ClientApp client;
        ASSERT_TRUE(client.init(argc, argv));

        std::thread client_thread(&clientWork, &client);

        probeLetter('E');
        probeLetter('D');

        client_thread.join();

        assertAttempts();

        client.deinit();
    }

    sleep(1);

    server.stop();
}

TEST_F(TestClientServer, ssfailed)
{
    WOF::NetServer server;
    const int serv_argc = 2;
    const char* serv_argv[2] = {"test", "-nattempts"};
    ASSERT_FALSE(server.start(serv_argc, serv_argv));
}

TEST_F(TestClientServer, csfailed)
{
    WOF::NetServer server;
    const int serv_argc = 1;
    const char* serv_argv[1] = {"test"};
    ASSERT_TRUE(server.start(serv_argc, serv_argv));

    const int argc = 2;
    const char* argv[2] = {"test", "-server"};

    {
        WOF::ClientApp client;
        ASSERT_FALSE(client.init(argc, argv));

        //client.deinit();
    }

    server.stop();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
