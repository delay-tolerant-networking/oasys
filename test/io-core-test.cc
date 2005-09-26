/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "io/IO.h"
#include "io/TCPClient.h"
#include "io/TCPServer.h"
#include "thread/Thread.h"
#include "util/UnitTest.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

const char* log = "/test/io";
size_t bufsz = 1048576;
char *buf1, *buf2;
struct iovec iov1[128];
struct iovec iov2[128];

int amounts1[10] = {
    3201, 21120, 10001, 4000, 551209, 70829, 97281, 99282, 102991, 88662
};

int amounts2[10] = {
    21120, 3201, 102991, 70829, 10001, 551209, 4000, 99282, 97281, 88662
};

int amounts3[209] = {
    2853, 3161, 3236, 8094, 1148, 7515, 5653, 6089, 4039, 7337, 4610, 9274, 4917, 3996, 4997, 7860, 7775, 7249, 7510, 1660, 3640, 4866, 9740, 4628, 2198, 2884, 458, 159, 8202, 9424, 9820, 5077, 7939, 3442, 1516, 8964, 3974, 947, 8992, 9509, 2375, 9887, 889, 8033, 3412, 7951, 3554, 5437, 775, 4159, 2509, 152, 7655, 2857, 8172, 1382, 6632, 8300, 1169, 3699, 9568, 1454, 2223, 6508, 8916, 8805, 1640, 7241, 4486, 5714, 7983, 9852, 2955, 5044, 9090, 389, 584, 9129, 7906, 6095, 6619, 6026, 5339, 6929, 5838, 3586, 2666, 9452, 7457, 5526, 9385, 4525, 8406, 7024, 4441, 6201, 9925, 2010, 9548, 2677, 2596, 84, 110, 5257, 7371, 429, 3384, 9794, 3464, 4914, 5775, 129, 9644, 1097, 460, 1365, 4879, 1857, 1731, 1900, 7334, 3029, 8436, 7145, 6917, 7276, 2309, 2704, 714, 5860, 8579, 1838, 8159, 9672, 1602, 4738, 9008, 3058, 4031, 6009, 4962, 1707, 9633, 3138, 2854, 3237, 547, 4647, 3399, 8670, 706, 7291, 1754, 2442, 5561, 7979, 7766, 7850, 1103, 320, 1196, 3207, 5282, 8658, 690, 762, 3441, 6961, 4416, 2500, 4504, 3095, 3820, 1774, 8992, 5587, 6460, 984, 9860, 3019, 6823, 7034, 991, 8162, 7400, 6388, 8011, 2626, 9468, 5974, 1015, 6397, 5737, 3401, 6867, 881, 9066, 3723, 2944, 5870, 9228, 7016, 3959, 9782, 2209, 2799, 9579, 4468, 1813
};

using namespace oasys;

DECLARE_TEST(Init) {
    buf1 = (char*)malloc(bufsz);
    buf2 = (char*)malloc(bufsz);

    for (size_t i = 0; i < bufsz; ++i) {
        buf1[i] = 'a' + (i % 26);
    }
    return UNIT_TEST_PASSED;
}

class ReadThread : public Thread {
public:
    ReadThread(int fd)
        : Thread("ReadThread", CREATE_JOINABLE), fd_(fd), done_(false) {}

    void run() {
        result_ = test();
        done_ = true;
    }

    int test() {
        char* bp;
        int cc;
        
        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 10; ++i) {
            cc = IO::readall(fd_, bp, amounts2[i], 0, log);
            CHECK_EQUAL(cc, amounts2[i]);
            usleep(100);
            bp += cc;
        }
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        CHECK_EQUAL(IO::readall(fd_, buf2, bufsz, 0, log), (int)bufsz);
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 209; ++i) {
            cc = IO::readall(fd_, bp, amounts3[i], 0, log);
            CHECK_EQUAL(cc, amounts3[i]);
            usleep(100);
            bp += cc;
        }
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 10; ++i) {
            cc = IO::readall(fd_, bp, amounts1[i], 0, log);
            CHECK_EQUAL(cc, amounts1[i]);
            usleep(100);
            bp += cc;
        }
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        return UNIT_TEST_PASSED;
    }

    int fd_;
    int result_;
    bool done_;
};

class WriteThread : public Thread {
public:
    WriteThread(int fd)
        : Thread("WriteThread", CREATE_JOINABLE), fd_(fd), done_(false) {}

    void run() {
        result_ = test();
        done_ = true;
    }

    int test() {
        int cc = IO::writeall(fd_, buf1, bufsz, 0, log);
    
        char* bp = buf1;
        for (int i = 0; i < 10; ++i) {
            cc = IO::writeall(fd_, bp, amounts1[i], 0, log);
            CHECK_EQUAL(cc, amounts1[i]);
            usleep(100);
            Thread::yield();
            bp += cc;
        }

        bp = buf1;
        for (int i = 0; i < 10; ++i) {
            cc = IO::writeall(fd_, bp, amounts2[i], 0, log);
            CHECK_EQUAL(cc, amounts2[i]);
            usleep(100);
            Thread::yield();
            bp += cc;
        }

        bp = buf1;
        for (int i = 0; i < 209; ++i) {
            cc = IO::writeall(fd_, bp, amounts3[i], 0, log);
            CHECK_EQUAL(cc, amounts3[i]);
            usleep(100);
            Thread::yield();
            bp += cc;
        }

        return UNIT_TEST_PASSED;
    }
    int fd_;
    int result_;
    bool done_;
};

class ReadvThread : public Thread {
public:
    ReadvThread(int fd)
        : Thread("ReadvThread", CREATE_JOINABLE), fd_(fd), done_(false) {}

    void run() {
        result_ = test();
        done_ = true;
    }

    int test() {
        char* bp;
        int cc;
        struct iovec iov[250];
        
        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 10; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len = amounts2[i];
            bp += amounts2[i];
        }
        cc = IO::readvall(fd_, iov, 10, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        iov[0].iov_base = buf2;
        iov[0].iov_len  = bufsz;
        cc = IO::readvall(fd_, iov, 1, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 209; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len = amounts3[i];
            bp += amounts3[i];
        }
        cc = IO::readvall(fd_, iov, 209, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        memset(buf2, 0, bufsz);
        bp = buf2;
        for (int i = 0; i < 10; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len = amounts1[i];
            bp += amounts1[i];
        }
        cc = IO::readvall(fd_, iov, 10, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);
        CHECK_EQUALSTRN(buf1, buf2, bufsz);

        return UNIT_TEST_PASSED;
    }

    int fd_;
    int result_;
    bool done_;
};

class WritevThread : public Thread {
public:
    WritevThread(int fd)
        : Thread("WritevThread", CREATE_JOINABLE), fd_(fd), done_(false) {}

    void run() {
        result_ = test();
        done_ = true;
    }

    int test() {
        int cc;
        struct iovec iov[250];
        iov[0].iov_base = buf1;
        iov[0].iov_len = bufsz;
        cc = IO::writevall(fd_, iov, 1, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);
    
        char* bp = buf1;
        for (int i = 0; i < 10; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len  = amounts1[i];
            bp += amounts1[i];
        }
        cc = IO::writevall(fd_, iov, 10, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);

        bp = buf1;
        for (int i = 0; i < 10; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len  = amounts2[i];
            bp += amounts2[i];
        }
        cc = IO::writevall(fd_, iov, 10, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);

        bp = buf1;
        for (int i = 0; i < 209; ++i) {
            iov[i].iov_base = bp;
            iov[i].iov_len  = amounts3[i];
            bp += amounts3[i];
        }
        cc = IO::writevall(fd_, iov, 209, 0, log);
        CHECK_EQUAL(cc, (int)bufsz);

        return UNIT_TEST_PASSED;
    }
    int fd_;
    int result_;
    bool done_;
};

int
rwvall_test(int* fds)
{
    int cc;

    // basic writeall/readall tests
    cc = IO::writeall(fds[1], buf1, 1024, 0, log);
    CHECK_EQUAL(cc, 1024);

    cc = IO::readall(fds[0], buf2, 1024, 0, log);
    CHECK_EQUAL(cc, 1024);
    
    CHECK_EQUALSTRN(buf1, buf2, 1024);

    // basic writevall/readvall
    iov1[0].iov_base = buf1;
    iov1[0].iov_len  = 32;
    iov1[1].iov_base = buf1 + 32;
    iov1[1].iov_len  = 64;
    iov1[2].iov_base = buf1 + 32 + 64;
    iov1[2].iov_len  = 4;
    iov1[3].iov_base = buf1 + 100;
    iov1[3].iov_len  = 924;
    
    memset(buf2, 0, 1024);
    iov2[0].iov_base = buf2;
    iov2[0].iov_len  = 32;
    iov2[1].iov_base = buf2 + 32;
    iov2[1].iov_len  = 64;
    iov2[2].iov_base = buf2 + 32 + 64;
    iov2[2].iov_len  = 4;
    iov2[3].iov_base = buf2 + 100;
    iov2[3].iov_len  = 924;
    
    cc = IO::writevall(fds[1], iov1, 4, 0, log);
    CHECK_EQUAL(cc, 1024);

    cc = IO::readvall(fds[0], iov2, 4, 0, log);
    CHECK_EQUAL(cc, 1024);
    
    CHECK_EQUALSTRN(buf1, buf2, 1024);

    // spark up the reader/writer threads in all four combinations

    // readall vs. writeall
    ReadThread   r(fds[0]);
    ReadvThread  rv(fds[0]);
    WriteThread  w(fds[1]);
    WritevThread wv(fds[1]);
    
    r.start();
    w.start();
    
    r.join();
    w.join();
    
    CHECK_EQUAL(r.done_, true);
    CHECK_EQUAL(r.result_, UNIT_TEST_PASSED);
    
    CHECK_EQUAL(w.done_, true);
    CHECK_EQUAL(w.result_, UNIT_TEST_PASSED);

    // readall vs. writevall...
    r.start();
    wv.start();
    
    r.join();
    wv.join();
    
    CHECK_EQUAL(r.done_, true);
    CHECK_EQUAL(r.result_, UNIT_TEST_PASSED);
    
    CHECK_EQUAL(wv.done_, true);
    CHECK_EQUAL(wv.result_, UNIT_TEST_PASSED);

    // readvall vs. writeall...
    rv.start();
    w.start();
    
    rv.join();
    w.join();
    
    CHECK_EQUAL(rv.done_, true);
    CHECK_EQUAL(rv.result_, UNIT_TEST_PASSED);
    
    CHECK_EQUAL(w.done_, true);
    CHECK_EQUAL(w.result_, UNIT_TEST_PASSED);

    // readvall vs. writevall...
    rv.start();
    wv.start();
    
    rv.join();
    wv.join();
    
    CHECK_EQUAL(rv.done_, true);
    CHECK_EQUAL(rv.result_, UNIT_TEST_PASSED);
    
    CHECK_EQUAL(wv.done_, true);
    CHECK_EQUAL(wv.result_, UNIT_TEST_PASSED);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(RwvallPipe) {
    int fds[2];
    CHECK_EQUAL(pipe(fds), 0);
    return rwvall_test(fds);
}

DECLARE_TEST(RwvallSocketpair) {
    int fds[2];
    CHECK_EQUAL(socketpair(AF_LOCAL, SOCK_STREAM, 0, fds), 0);
    return rwvall_test(fds);
}

DECLARE_TEST(RwvallTCP) {
    int fds[2];
    in_addr_t addr;
    u_int16_t port;

    TCPServer server;
    TCPClient client;
    client.init_socket();
    
    CHECK_EQUAL(server.bind(htonl(INADDR_LOOPBACK), 10000), 0);
    CHECK_EQUAL(server.listen(), 0);
    CHECK_EQUAL(client.set_nonblocking(true), 0);
    CHECK_EQUAL(client.connect(htonl(INADDR_LOOPBACK), 10000), -1);
    CHECK_EQUAL(errno, EINPROGRESS);
    CHECK_EQUAL(server.accept(&fds[1], &addr, &port), 0);
    CHECK_EQUAL(client.set_nonblocking(false), 0);
    fds[0] = client.fd();
    
    return rwvall_test(fds);
}

DECLARE_TESTER(IoCoreTester) {
    ADD_TEST(Init);
    ADD_TEST(RwvallPipe);
    ADD_TEST(RwvallSocketpair);
    ADD_TEST(RwvallTCP);
}

DECLARE_TEST_FILE(IoCoreTester, "IO Core Test");
