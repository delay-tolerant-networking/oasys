#ifndef _OASYS_BT_INQUIRY_H_
#define _OASYS_BT_INQUIRY_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h> 

#include "../debug/Log.h"

#define MAX_BTNAME_SZ 248

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

namespace oasys {

class BluetoothInquiry : public Logger {

public:

#define BT_INQ_NUM_RESP 20
#define BT_INQ_LENGTH 8

    BluetoothInquiry(const char * logbase = "/dtn/btinquiry");
    ~BluetoothInquiry();

    /*
     * Perform inquiry action
     */
    int inquire();

    /*
     * Enumerate over discovered device adapter addresses
     */
    int first(bdaddr_t&);
    int next(bdaddr_t&);

protected:
    void reset();  // performed internally between inquiries

    // state and iterator over info_
    int num_responses_i_, pos_;
    // buffer for query data
    inquiry_info info_[BT_INQ_NUM_RESP];
    // flags to change default inquiry behavior
    long flags_;
};

}
#endif // OASYS_BLUETOOTH_ENABLED
#endif // _OASYS_BT_INQUIRY_H_
