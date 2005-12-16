#include <unistd.h>

#include "util/UnitTest.h"

#include "smtp/BasicSMTP.h"
#include "smtp/SMTPClient.h"
#include "smtp/SMTPServer.h"

using namespace oasys;

class MySMTPConfig : public SMTP::Config {
public:
    MySMTPConfig() :
        Config(htonl(INADDR_LOOPBACK), 17760, -1, "test.domain.com") {}
};
MySMTPConfig config;

typedef std::vector<BasicSMTPMsg> MailList;
MailList ml;

BasicSMTPMsg msgs[3] = {
    BasicSMTPMsg("<foo1@foo1.com>", "<bar1@bar.com>, <bar2@bar.com>",
                 "line 01: This is the body of the message\r\n"
                 "line 02: This is the body of the message\r\n"
                 "line 03: This is the body of the message\r\n"
                 "line 04: This is the body of the message\r\n"
                 "line 05: This is the body of the message\r\n"
                 "line 06: This is the last line\r\n"),

    BasicSMTPMsg("<foo2@foo2.org>", "<bar3@bar.com>, <bar4@bar.com>",
                 "\r\n"
                 "line 01: This is a message with a leading newline\r\n"
                 "\r\n"
                 ".\r\n"
                 "\r\n"
                 ".line 02: This is a line with a leading dot\r\n"),
    
    BasicSMTPMsg("<foo3@foo3.net>", "<bar5@bar.com>, <bar6@bar.com>", ".\r\n")
};

class TestSMTPHandler : public BasicSMTPHandler {
public:
    virtual void message_recvd(const BasicSMTPMsg& msg) {
        ml.push_back(msg);
    }
};

class TestSMTPFactory : public SMTPHandlerFactory {
public:
    SMTPHandler* new_handler() { return new TestSMTPHandler(); }
};

DECLARE_TEST(SmtpPipe) {
    int pipe1[2];
    int pipe2[2];

    CHECK(pipe(pipe1) == 0);
    CHECK(pipe(pipe2) == 0);

    Notifier done("SmtpPipe::done");
    SMTPHandlerThread* t =
        new SMTPHandlerThread(new TestSMTPHandler(), pipe1[0], pipe2[1],
                              SMTP::DEFAULT_CONFIG, &done);
    t->start();

    SMTPFdClient c(pipe2[0], pipe1[1]);
    for (int i = 0; i < 3; ++i) {
        BasicSMTPSender s("test.domain.com", &msgs[i]);
        CHECK(msgs[i].valid());
        CHECK_EQUAL(c.send_message(&s), 0);
    }

    close(pipe2[0]);
    close(pipe1[1]);

    done.wait();
    while (! t->is_stopped()) {
        usleep(200);
    }

    close(pipe1[0]);
    close(pipe2[1]);
    
    return UNIT_TEST_PASSED;
}

TestSMTPFactory f;
SMTPServer* server;
Notifier* session_done;

DECLARE_TEST(StartServer) {
    session_done = new Notifier("SessionDone");
    server = new SMTPServer(config, &f, session_done, 500);
    server->start();

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(StopServer) {
    server->set_should_stop();
    while (! server->is_stopped()) {
        usleep(200);
    }

    server->close();

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(SmtpSockets) {
    SMTPClient c;
    CHECK(c.timeout_connect(config.addr_, config.port_, config.timeout_) == 0);
    
    for (int i = 0; i < 3; ++i) {
        BasicSMTPSender s("test.domain.com", &msgs[i]);
        CHECK(msgs[i].valid());
        CHECK_EQUAL(c.send_message(&s), 0);
    }

    c.close();
    session_done->wait();

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(SmtpPython) {
    CHECK(system("python ./smtp-test-send.py") == 0);
    session_done->wait();
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(SmtpTcl) {
    CHECK(system("tclsh ./smtp-test-send.tcl") == 0);
    session_done->wait();
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(CheckMsgs) {
    CHECK(ml.size() == 3);

    for (int i = 0; i < 3; ++i) {
        CHECK_EQUALSTR(ml[i].from_.c_str(), msgs[i].from_.c_str());
        CHECK_EQUAL(ml[i].to_.size(), msgs[i].to_.size());
        CHECK_EQUAL(ml[i].to_.size(), 2);
        
        CHECK_EQUALSTR(ml[i].to_[0].c_str(), msgs[i].to_[0].c_str());
        CHECK_EQUALSTR(ml[i].to_[1].c_str(), msgs[i].to_[1].c_str());

        // nice hack mike...
        if (ml[i].msg_.length() > 4 && ml[i].msg_.substr(0, 4) == "MIME") {
            // no comparison
        } else {
            CHECK_EQUALSTR(ml[i].msg_.c_str(), msgs[i].msg_.c_str());
        }
    }

    ml.clear();
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Tester) {
    ADD_TEST(StartServer);
    ADD_TEST(SmtpPipe);
    ADD_TEST(CheckMsgs);
    ADD_TEST(SmtpSockets);
    ADD_TEST(CheckMsgs);
    ADD_TEST(SmtpPython);
    ADD_TEST(CheckMsgs);
    ADD_TEST(SmtpTcl);
    ADD_TEST(CheckMsgs);
    ADD_TEST(StopServer);
}

DECLARE_TEST_FILE(Tester, "SMTP test");
