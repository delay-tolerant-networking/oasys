/* $Id$ */

#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>

#include "../debug/Log.h"
#include "Bluetooth.h"
#include "BluetoothInquiry.h"

namespace oasys {

BluetoothInquiry::BluetoothInquiry(const char * logbase)
    : Logger("BluetoothInquiry", logbase),
      hci_dev_(0),
      num_responses_(10),
      num_responses_i_(-1),
      length_(8),
      lap_(NULL),
      info_(NULL),
      flags_(0L),
      timeout_(100000),
      fd_(-1),
      pos_(0)
{
    hci_device_name_ = NULL;
    set_hci_device_name("hci0");
}

BluetoothInquiry::~BluetoothInquiry()
{
    if(fd_ != -1)
    {
close(fd_);
    }

    if(info_)
    {
        free(info_);
    }
    delete [] hci_device_name_;
}

char *
BluetoothInquiry::hci_device_name()
{
    return hci_device_name_;
}

void
BluetoothInquiry::set_hci_device_name(char *hci_device_name )
{
    int sz = strlen(hci_device_name) + 1;
    if (hci_device_name_) {
        delete [] hci_device_name_;
    }
    hci_device_name_ = new char[sz];
    strncpy(hci_device_name_, hci_device_name, sz);
    hci_dev_ = Bluetooth::hci_devid(hci_device_name_,logpath_);
}

int
BluetoothInquiry::num_responses()
{
    return num_responses_;
}

void
BluetoothInquiry::set_numresponses(int nr)
{
    ASSERT( nr > 0 && nr < 250 );
    num_responses_ = nr;
}

int
BluetoothInquiry::length()
{
    return length_;
}

void
BluetoothInquiry::set_length(int length)
{
    ASSERT(length > 0 && length < 20);
    length_ = length;
}

uint8_t *
BluetoothInquiry::lap()
{
    return lap_;
}

void
BluetoothInquiry::set_lap(uint8_t *lap )
{
    lap_ = lap;
}

inquiry_info *
BluetoothInquiry::info()
{
    return info_;
}

void
BluetoothInquiry::set_info(inquiry_info *info)
{
    info_ = info;
}

long
BluetoothInquiry::flags()
{
    return flags_;
}

void
BluetoothInquiry::set_flags(long flags)
{
    flags_ = flags;
}
 
int
BluetoothInquiry::inquire()
{
    if(info_)
    {
        free(info_);
        info_ = NULL;
    }
    num_responses_i_ = Bluetooth::hci_inquiry(
                           hci_dev_,length_,num_responses_,
                           lap_,&info_,flags_,logpath_);
    if( num_responses_i_ < 0 )
    {
        log_info("hci_inquiry found no devices in range");
    }
    return num_responses_i_;
}

int
BluetoothInquiry::timeout()
{
    return timeout_;
}
 
void
BluetoothInquiry::set_timeout(int to)
{
    timeout_ = to;
}

void
BluetoothInquiry::reset()
{
    pos_ = 0;
    if( fd_ != -1 ) {
        close(fd_);
        fd_ = -1;
    }
    flags_ |= IREQ_CACHE_FLUSH;
}

int
BluetoothInquiry::first(BluetoothInquiryInfo& bii)
{
    reset();
    return next(bii);
}

int
BluetoothInquiry::next(BluetoothInquiryInfo& bii)
{
    char buff[18];
    ASSERT( info_ != NULL );
    if(pos_ >= num_responses_i_)
    {
        return -1;
    }
    log_debug("BluetoothInquiry::next(%d)",pos_);
    if(fd_ == -1)
    {
        fd_ = Bluetooth::hci_open_dev(hci_dev_,logpath_);
        if(fd_ == -1)
        {
            log_err("hci_open_dev failed");
            return -1;
        }
    }
    memset(bii.name_,0,bii.name_len_);
    if(Bluetooth::hci_read_remote_name(fd_,&(info_ + pos_)->bdaddr,
                                   bii.name_len_,bii.name_,
                                   timeout_) < 0 )
    {
        strcpy(bii.name_,"[none]");
    }
    bacpy(&bii.addr_,&(info_ + pos_)->bdaddr); 
    log_debug("read remote name %s(%s)",bii.name_,
              Bluetooth::batostr(&bii.addr_,buff));
    pos_++;
    return 0;
}

} // namespace oasys
#endif /* OASYS_BLUETOOTH_ENABLED */
