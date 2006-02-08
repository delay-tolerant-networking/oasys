/* $Id$ */
#ifndef _OASYS_BT_INQUIRY_H_
#define _OASYS_BT_INQUIRY_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h> 

#include "../debug/Log.h"

namespace oasys {

class BluetoothInquiryInfo {
public:
    BluetoothInquiryInfo(char* name="", int name_len=248) 
        : name_(name),
          name_len_(name_len)
    {
        memset(&addr_,0,sizeof(addr_));
        name_ = new char[name_len_];
        memset(name_,0,name_len_);
    }

    BluetoothInquiryInfo(const BluetoothInquiryInfo& btc)
    { 
        memset(&addr_,0,sizeof(addr_));
        bacpy(&this->addr_,&btc.addr_);

        name_len_ = btc.name_len_;
        name_ = new char[name_len_];
        memset(name_,0,name_len_);
        strncpy(name_,btc.name_,name_len_);
    }

    ~BluetoothInquiryInfo()
    {
        delete [] name_;
    }

    char *name_;
    int name_len_;
    bdaddr_t addr_;
};

class BluetoothInquiry : public Logger {

public:
    BluetoothInquiry(const char * logbase = "/btinquiry");
    ~BluetoothInquiry();

    /*
     * Set/get inquiry parameters
     */
    char * hci_device_name();
    void set_hci_device_name(char*);

    /// number of desired responses to wait for
    int num_responses();
    void set_numresponses(int);

    /// number of 1.28s intervals to scan
    int length();
    void set_length(int);

    /// lower address part ... see p. 64, vol 1, part B of Bluetooth 2.0
    uint8_t* lap();
    void set_lap(uint8_t*);

    inquiry_info* info();
    void set_info(inquiry_info*);
    
    long flags();
    void set_flags(long);
    
    /*
     * Perform inquiry action
     */
    int inquire();

    /*
     * Set/get read_remote_name parameters
     */
    int timeout();
    void set_timeout(int);

    /*
     * Perform read_remote_name action
     */
    int first(BluetoothInquiryInfo& bti);
    int next(BluetoothInquiryInfo& bti);

protected:
    void reset();  // performed internally between inquiries

    char *hci_device_name_;
    int hci_dev_;
    int num_responses_,num_responses_i_;
    int length_;
    uint8_t *lap_;
    inquiry_info *info_;
    long flags_;
    int timeout_;

    int fd_;
    int pos_;
};

}
#endif // OASYS_BLUETOOTH_ENABLED
#endif // _OASYS_BT_INQUIRY_H_
