#include "config.h"

#ifdef OASYS_BLUETOOTH_ENABLED

#include "Bluetooth.h"
#include "RFCOMMClient.h"
#include <errno.h>

extern int errno;

namespace oasys {

int
RFCOMMClient::rc_connect(bdaddr_t remote_addr)
{
    int res = -1;
    char buff[18];

    set_remote_addr(remote_addr);

    // start at 1 work up to 30
    channel_ = 1;
    while (channel_ <= 30) {

        if ((res = bind()) != 0) {


            // something is borked
            if (errno != EADDRINUSE) {
                log_err("error binding to %s:%d: %s",
                        Bluetooth::batostr(&local_addr_,buff),
                        channel_,
                        strerror(errno));

                // unrecoverable
                if (errno == EBADFD) {
                    close();
                    return -1;
                }
                break;
            }

            log_debug("can't bind to %s:%d: %s",
                      Bluetooth::batostr(&local_addr_,buff),
                      channel_,
                      strerror(errno));

        } else {

            // local bind succeeded, now try remote connect

            if ((res = connect()) == 0) {

                // success!
                log_debug("connected to %s:%d",
                          Bluetooth::batostr(&remote_addr_,buff),
                          channel_);

                return res;

            } else {

                // failed to connect; report it and move on
                log_debug("can't connect to %s:%d: %s",
                          Bluetooth::batostr(&remote_addr_,buff),
                          channel_,
                          strerror(errno)); 

                // unrecoverable
                if (errno == EBADFD) {
                    close();
                    return -1;
                }

            }
        }

        // this channel's busy, try the next
        ++channel_;
    }

    log_err("Scanned all RFCOMM channels but unable to connect to %s",
            Bluetooth::batostr(&remote_addr_,buff));
    return -1;
}

int
RFCOMMClient::rc_connect()
{
    return rc_connect(remote_addr_);
}


}  // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
