#include <vector>

#include <io/IOClient.h>
#include <io/BufferedIO.h>

namespace oasys {

class TestClient : public IOClient {
public:
    TestClient() : read_num_(0) {}

    int read(char* bp, size_t len) {
        int cc = sprintf(bp, "read(%d)endl\n", read_num_);
        read_num_++;

        if(read_num_ > 5)
            return 0;

        return cc;
    }

    int readv(const struct iovec* iov, int iovcnt) {
        return 0;
    }

    int readall(char* bp, size_t len) {
        return 0;
    }

    int readvall(const struct iovec* iov, int iovcnt) {
        return 0;
    }
    
    int write(const char* bp, size_t len) {
        printf("write: \"%s\"\n", bp);

        return len;
    }

    int writev(const struct iovec* iov, int iovcnt) {
        return 0;
    }

    int writeall(const char* bp, size_t len) {
        return 0;
    }

    int writevall(const struct iovec* iov, int iovcnt) {
        return 0;
    }
    
    int timeout_read(char* bp, size_t len, int timeout_ms) {
        return 0;
    }

    int timeout_readall(char* bp, size_t len, int timeout_ms) {
        return 0;
    }

    int timeout_readv(const struct iovec* iov, int iovcnt,
                      int timeout_ms) {
        return 0;
    }
            
    int timeout_readvall(const struct iovec* iov, int iovcnt,
                         int timeout_ms) {
        return 0;
    }

    int set_nonblocking(bool b) {
        return 0;
    }
            
    int get_nonblocking(bool* b) {
        return 0;
    }
            
private:
    int read_num_;
};

}

using namespace oasys;

int
main(int argc, char* argv[])
{
    Log::init(LOG_DEBUG);
    BufferedInput b(new TestClient());

    char* buf;
    for(int i=0; i<7; ++i) 
    {
        int cc;

        if((cc = b.read_line("endl", &buf)) == 0) 
        {
            printf("%d: read eof\n", i);
        }
        else
        {
            char print_buf[100];

            strncpy(print_buf, buf, cc);
            
            printf("%d: %d, \"%s\"\n", i, cc, print_buf);
        }
    }
}
