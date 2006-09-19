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

    set_remote_addr(remote_addr);

    for (channel_ = 1; channel_ <= 30; channel_++) {

        if ((res = bind()) != 0) { 

            close();

            if (errno != EADDRINUSE) {
                // something is borked
                if ( params_.silent_connect_ == false )
                    log_err("error binding to %s:%d: %s",
                            bd2str(local_addr_), channel_, strerror(errno));

                // unrecoverable
                if (errno == EBADFD) {
                    //return -1;
                }

                break;
            }

            if ( params_.silent_connect_ == false )
                log_debug("can't bind to %s:%d: %s",
                          bd2str(local_addr_), channel_, strerror(errno));

        } else {

            // local bind succeeded, now try remote connect

            if ((res = connect()) == 0) {

                // success!
                if ( params_.silent_connect_ == false )
                    log_debug("connected to %s:%d",
                              bd2str(remote_addr_), channel_);

                return res;

            } else {

                close();

                // failed to connect; report it and move on
                if ( params_.silent_connect_ == false )
                    log_debug("can't connect to %s:%d: %s",
                              bd2str(remote_addr_), channel_, strerror(errno)); 

                // unrecoverable
                if (errno == EBADFD) {
                    //return -1;
                }

            }
        }
    }

    log_err("Scanned all RFCOMM channels but unable to connect to %s",
            bd2str(remote_addr_));
    return -1;
}

int
RFCOMMClient::rc_connect()
{
    ASSERT(bacmp(&remote_addr_,BDADDR_ANY)!=0);
    return rc_connect(remote_addr_);
}


}  // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
