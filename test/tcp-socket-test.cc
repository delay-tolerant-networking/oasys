
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <debug/Debug.h>
#include <debug/Log.h>
#include <thread/Thread.h>
#include <io/NetUtils.h>
#include <io/TCPClient.h>
#include <io/TCPServer.h>

using namespace oasys;

#define PORT 17600

// The client side socket that initiates the ping
class TestTcpPing : public TCPClient, public Thread {
public:
    TestTcpPing() : TCPClient()
    {
    }
    
protected:
    void run()
    {
        sleep(1);
        
        while (1) {
            connect(htonl(INADDR_LOOPBACK), PORT);
            for (int i = 0; i < 5; ++i) {
                snprintf(outpkt_, sizeof(outpkt_), "ping %d", i);
                write(outpkt_, strlen(outpkt_));
                int cc = read(inpkt_, sizeof(inpkt_));
                log_info("got packet '%s' of size %d", inpkt_, cc);
                sleep(1);
            }
            close();
        }
    }

public:
    char inpkt_[64];
    char outpkt_[64];
};

class TestTcpPong : public TCPClient, public Thread {
public:
    // The server side accept()ed socket that responds
    TestTcpPong(int fd, in_addr_t host, u_int16_t port) :
        TCPClient(fd, host, port),Thread(DELETE_ON_EXIT)
    {
        snprintf(outpkt_, sizeof(outpkt_), "pong");
    }

    ~TestTcpPong()
    {
        log_info("pong thread exiting");
    }

    void run()
    {
        while (1) {
	    memset(inpkt_, 0, sizeof(inpkt_));
            int cc = read(inpkt_, sizeof(inpkt_));
            if (cc == 0) {
                close(); // eof
                return;
            }
            
            log_info("got packet '%s' of size %d", inpkt_, cc);
            write(outpkt_, strlen(outpkt_));
        }
    }
    
    char inpkt_[64];
    char outpkt_[64];
};

class TestTcpServer : public TCPServerThread {
public:
    TestTcpServer()
        : TCPServerThread("/test-server")
    {
        log_info("starting up");
        bind(htonl(INADDR_LOOPBACK), PORT);
        listen();
        start();
    }

    void accepted(int fd, in_addr_t addr, u_int16_t port)
    {
        TestTcpPong* p = new TestTcpPong(fd, addr, port);
        p->start();
    }
};

int
main(int argc, const char** argv)
{
    in_addr_t addr;

    Log::init(LOG_INFO);

    log_info("/test", "testing gethostbyname");
    if (gethostbyname("10.0.0.1", &addr) != 0) {
        log_err("/test", "error: can't gethostbyname 10.0.0.1");
    }
    if (addr != inet_addr("10.0.0.1")) {
        log_err("/test", "error: gethostbyname 10.0.0.1 got %x, not %x",
                addr, inet_addr("10.0.0.1"));
    }
    
    if (gethostbyname("localhost", &addr) != 0) {
        log_err("/test", "error: can't gethostbyname localhost");
    }
    
    if (ntohl(addr) != INADDR_LOOPBACK) {
        log_err("/test", "error: gethostbyname(localhost) got %x, not %x",
                addr, INADDR_LOOPBACK);
    }

    TestTcpServer* s = new TestTcpServer();
    s->start();

    TestTcpPing* p = new TestTcpPing();
    p->start();

    s->join();
}
