/* $Id$ */
#ifndef _OASYS_BTSDP_H_
#define _OASYS_BTSDP_H_

#include <config.h> 
#ifdef OASYS_BLUETOOTH_ENABLED

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "../debug/Log.h"

// generated using uuidgen on Mac OS X ... a completely arbitrary number :)
// maybe eventually register something with Bluetooth SIG?
// the joke here: why UUID?  why not call it GUID?  Cuz the SIG knows best
#define OASYS_BLUETOOTH_SDP_UUID { 0xDCA3, 0x8352, 0xBF60, 0x11DA, \
                                   0xA23B, 0x0003, 0x931B, 0x7960 }

namespace oasys {

// for reference, from <bluetooth/sdp.h>
#if 0

typedef struct _sdp_list sdp_list_t;
struct _sdp_list {
    sdp_list_t *next;
    void *data;
};

#endif

/* Object to manage linked list of type sdp_list_t */
class SDPListHead
{
public:
    SDPListHead() :
        free_func_(NULL), head_(NULL), current_(NULL)
    {
    }

    SDPListHead(sdp_list_t *head)
        : free_func_(NULL), current_(NULL)
    {
        head_ = head;
    }

    ~SDPListHead()
    {
        free_list();
    }

    // is there some special way to reclaim memory for each element 
    // of this list?  save a pointer to that function here
    void set_free_func(sdp_free_func_t f) { free_func_ = f; }

    // get/set pointer to the head of the list
    sdp_list_t* head() { return head_; }
    void head(sdp_list_t* head) { free_list(); head_ = head; }

    // iterate over elements of list
    // fails with NULL when end of list is reached
    // next call (after NULL) resets to head of list
    sdp_list_t* next() {
        if (current_) {
            current_ = current_->next;
            return current_;
        }
        current_ = head_;
        return current_;
    }

protected:
    void free_list()
    {
        // below is lifted straight from BlueZ's sdp.c, with minor mods
        while (head_) {
            // save pointer to next
            sdp_list_t *next = head_->next;
            // if there's a special way to free data, then execute
            if (free_func_)
                free_func_(head_->data);
            // free this element
            free(head_);
            // increment to next
            head_ = next;
        }
    }

    sdp_free_func_t  free_func_;
    sdp_list_t      *head_;
    sdp_list_t      *current_;
};

/**
 * Connect to remote Bluetooth device and query its SDP server
 * for DTN service
 */
class BluetoothServiceDiscoveryClient : public Logger
{
public:
    BluetoothServiceDiscoveryClient(const char* logpath = "/dtn/bt/sdp/client");
    ~BluetoothServiceDiscoveryClient();

    bool is_dtn_router(bdaddr_t addr);
protected:
    /* iterator over query results */
    sdp_record_t* get_next_service_record();

    /* Manage connection to remote SDP service */
    bool          connect();
    bool          close();

    /* Connect to remote SDP server and return query results */
    sdp_list_t*   do_search();

    /* member data */
    bdaddr_t      remote_addr_;     /* physical address of device to query */
    SDPListHead   *response_list_;  /* linked list of SDP responses */
    sdp_session_t *session_handle_; /* handle to open search request */
};

class BluetoothServiceRegistration : public Logger 
{
public:
    BluetoothServiceRegistration(const char* logpath = "/dtn/bt/sdp/reg");
    ~BluetoothServiceRegistration();

    bool success() {return status_;};
protected:
    bool register_service();

    sdp_session_t* session_handle_;
    bool status_;
};

} // namespace oasys

#endif /* OASYS_BLUETOOTH_ENABLED */
#endif /* _OASYS_BTSDP_H_ */
