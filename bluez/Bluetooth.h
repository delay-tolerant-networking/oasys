/* $Id$ */ 
#ifndef _OASYS_BT_H_
#define _OASYS_BT_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>

#include <vector>
using namespace std;

namespace oasys {

struct Bluetooth {

#ifndef HCIDEVNAMSIZ
#define HCIDEVNAMSIZ 32
#endif

    //@{
    /// System call wrappers (for logging)
    static int hci_devid(const char* hcidev, 
                         const char* log = NULL );
    
    static int hci_inquiry(int dev_id, int len, int nrsp, 
                           const uint8_t *lap, inquiry_info **ii, 
                           long flags, const char* log = NULL );

    static int hci_open_dev(int dev_id,
                            const char* log = NULL );

    static int hci_close_dev(int dd,
                             const char* log = NULL );

    static int hci_read_remote_name(int dd, const bdaddr_t *bdaddr, 
                                    int len, char *name, int to,
                                    const char* log = NULL );

    static void hci_get_bdaddr(const char *hcidev, bdaddr_t *bdaddr,
                               const char *log = NULL);

    static int hci_dev_up(int dd, const char *hcidev,
                          const char *log = NULL);
    //@}
    
    static char * batostr(const bdaddr_t *ba, char * str, size_t strsize = 18);

    static bdaddr_t * strtoba(const char *str, bdaddr_t *addr);

    static void baswap(bdaddr_t *dst, const bdaddr_t *src);
    
    /*
       Helper class for Bluetooth addresses
     */
    class btaddr {
    public:
       btaddr ()
       {
           memset(&addr_,0,sizeof(bdaddr_t));
           channel_=0;
           memset(hcidev_,0,HCIDEVNAMSIZ);
       }
       bdaddr_t addr_;
       u_int8_t channel_;
       char hcidev_[HCIDEVNAMSIZ];
    };

}; // struct Bluetooth

} // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_BT_H_ */
