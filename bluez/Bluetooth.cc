/* $Id$ */

#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
extern int errno;

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>

#include "Bluetooth.h"
#include "debug/Log.h"

namespace oasys {

int
Bluetooth::hci_devid(const char* hcidev, const char* log)
{
    int dd = ::hci_devid(hcidev);
    if(log)
    {
        logf(log, LOG_INFO, "hci_devid %s: dd %d", hcidev, dd);
    }
    return dd;
}

int
Bluetooth::hci_inquiry(int dev_id, int len, int nrsp, const uint8_t *lap, 
                       inquiry_info **ii, long flags, const char* log)
{
    int err = ::hci_inquiry(dev_id,len,nrsp,lap,ii,flags);
    if(log)
    {
        logf(log, LOG_INFO, 
             "hci_inquiry(hci%d): len %d, nrsp %d, lap %p, info %p, flags 0x%lx",
             dev_id,len,nrsp,lap,ii,flags);
    }
    return err;
}

int
Bluetooth::hci_open_dev(int dev_id, const char* log)
{
    int fd = ::hci_open_dev(dev_id);
    if(log)
    {
        logf(log, LOG_INFO, "hci_open_dev(hci%d): fd %d",dev_id,fd);
    }
    return fd;
}

int
Bluetooth::hci_close_dev(int dd, const char* log)
{
    int err = ::hci_close_dev(dd);
    if(log)
    {
        logf(log, LOG_INFO, "hci_close_dev(%d): err %d",dd,err);
    }
    return err;
}

int
Bluetooth::hci_read_remote_name(int dd, const bdaddr_t *bdaddr, int len, 
                                char *name, int to, const char* log)
{
    int err = ::hci_read_remote_name(dd,bdaddr,len,name,to);
    if(log)
    { 
        bdaddr_t ba;
        baswap(&ba,bdaddr);
        char buff[18];
        logf(log, LOG_INFO, 
             "hci_read_remote_name(%d): [%s] %s len %d to %d",
             dd,batostr(&ba,buff),name,len,to);
    }
    return err;
}

void
Bluetooth::hci_get_bdaddr(const char * hcidev, bdaddr_t* bdaddr,
                          const char * log)
{
    struct hci_dev_info di;

    // open socket to HCI control interface
    int fd=socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (fd<0)
    {
        if (log) logf(log, LOG_ERR, "can't open HCI socket");
        return;
    }
    int dev_id = hci_devid(hcidev);
    if (dev_id < 0)
    {
        if (log)
            logf(log, LOG_INFO,
                 "bad device id -- attempting to up the interface");
        if (hci_dev_up(fd,hcidev,log) < 0)
        {
            if (log)
                logf(log, LOG_ERR, "unable to change device status: %s",
                     hcidev);
            return;
        }
        if ((dev_id = hci_devid(hcidev)) < 0) {
            return;
        }
    }
    di.dev_id = dev_id;
    if (ioctl(fd, HCIGETDEVINFO, (void *) &di) < 0)
    {
        if(log) logf(log, LOG_ERR, "can't get device info");
        return;
    }
    bacpy(bdaddr,&di.bdaddr);
    close(fd);
}

int
Bluetooth::hci_dev_up(int dd, const char * hcidev, const char *log)
{
    int dev_id=-1;
    if (strncmp(hcidev,"hci",3) == 0 && strlen(hcidev) >= 4)
    {
        dev_id = atoi(hcidev+3);
    }
    if (dev_id<0)
    {
        if (log) logf(log, LOG_ERR, "badly formatted HCI device name: %s",hcidev);
        return -1;
    }
    if (ioctl(dd, HCIDEVUP, dev_id) < 0)
    {
        if (log) logf(log, LOG_ERR, "failed to init device hci%d: %s (%d)",
                      dev_id, strerror(errno), errno);
        return -1;
    }
    return 0;
}

/* Modified from BlueZ's bluetooth.c */
char *
Bluetooth::batostr(const bdaddr_t* ba, char *str, size_t str_size)
{
    if (!str)
        return NULL;

    memset(str,0,str_size);

    snprintf(str, str_size, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
             ba->b[5], ba->b[4], ba->b[3], 
             ba->b[2], ba->b[1], ba->b[0]);

    return str;
}

/* Modified from BlueZ's bluetooth.c */
bdaddr_t *
Bluetooth::strtoba(const char *str, bdaddr_t *addr)
{
    const char *ptr = str;
    int i;

    if (!addr)
        return NULL;
    bdaddr_t bd;
    uint8_t *ba = (uint8_t*) &bd;

    for(i = 0; i < 6; i++) {
        ba[i] = (uint8_t) strtol(ptr, NULL, 16);
        if (i != 5 && !(ptr = strchr(ptr,':')))
            ptr = ":00:00:00:00:00";
        ptr++;
    }

    baswap(addr,(bdaddr_t *)ba);
    return addr;
}

void
Bluetooth::baswap(bdaddr_t *dst, const bdaddr_t *src)
{
    unsigned char *d = (unsigned char *) dst;
    const unsigned char *s = (const unsigned char *) src;
    int i;
    for (i = 0; i < 6; i++)
        d[i] = s[5-i];
}


} // namespace oasys
#endif /* OASYS_BLUETOOTH_ENABLED */