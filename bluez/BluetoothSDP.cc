#include <config.h>
#ifdef OASYS_BLUETOOTH_ENABLED

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

extern int errno;

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/rfcomm.h>

#include "BluetoothSDP.h"
//#include <debug/Logger.h>

namespace oasys {

BluetoothServiceDiscoveryClient::
BluetoothServiceDiscoveryClient(const char* logpath) :
    Logger("BluetoothServiceDiscoveryClient",logpath),
    response_list_(NULL),
    session_handle_(NULL)
{
    bacpy(&local_addr_,BDADDR_ANY);
}

BluetoothServiceDiscoveryClient::
~BluetoothServiceDiscoveryClient()
{
    // terminate connection to remote service
    close();

    // clean up internal data structures
    if (response_list_) {
        delete response_list_;
        response_list_ = NULL;
    }
}

bool
BluetoothServiceDiscoveryClient::
connect()
{
    if (session_handle_ != NULL) return true;

    // connect to the SDP server running on the remote machine
    session_handle_ = sdp_connect(
                        &local_addr_, /* bind to specified local adapter */
                        &remote_addr_,
                        SDP_RETRY_IF_BUSY);

    if ( ! session_handle_ ) {
        // could be a device that does not implement SDP
        log_debug("failed to connect to SDP server: %s (%d)\n",
                  strerror(errno), errno);
        return false;
    }

    return true;
}

bool
BluetoothServiceDiscoveryClient::
close()
{
    if (session_handle_) {
        sdp_close(session_handle_);
        session_handle_ = NULL;
        return true;
    }
    return false;
}

sdp_list_t*
BluetoothServiceDiscoveryClient::
do_search() {

    // connect to SDP service on remote system
    if (connect()) {

        // specify Universally Unique Identifier of service to query for
        const uint32_t dtn_svc_uuid_int[] = OASYS_BLUETOOTH_SDP_UUID;
        uuid_t svc_uuid;

        // hi-order (0x0000) specifies start of search range;
        // lo-order (0xffff) specifies end of range;
        // 0x0000ffff specifies a search of full range of attributes
        uint32_t range = 0x0000ffff;

        // specify DTN's UUID, which is the application we're searching for
        sdp_uuid128_create(&svc_uuid,&dtn_svc_uuid_int);

        // initialize the linked list with DTN's UUID to limit SDP 
        // search scope on remote host
        sdp_list_t *search = sdp_list_append(NULL,&svc_uuid);

        // search all attributes by specifying full range
        sdp_list_t *attrid = sdp_list_append(NULL,&range);

        // list of service records returned by search request
        sdp_list_t *response_list;

        // get a list of service records that have matching UUID
        int err = sdp_service_search_attr_req(
                    session_handle_,     /* open session handle        */
                    search,              /* define the search (UUID)   */
                    SDP_ATTR_REQ_RANGE,  /* type of search             */
                    attrid,              /* search mask for attributes */      
                    &response_list);     /* linked list of responses   */

        // that's all that we do with this connection
        //close();

        // manage the malloc()'s flying around like crazy in BlueZ
        sdp_list_free(attrid,0);
        sdp_list_free(search,0);

        if (err != 0) {
            log_debug("problems with sdp search: %s (%d)\n",
                      strerror(errno),errno);
            return NULL;
        }

        return response_list;
    }

    // connect() already reported error
    return NULL;
}

sdp_record_t *
BluetoothServiceDiscoveryClient::
get_next_service_record()
{

    // response_list_ points to the head of the linked list of records
    // if this is the first call, then connect and initiate the search
    if (response_list_ == NULL) {
        sdp_list_t* search = do_search();
        if (search == NULL) return NULL;
        response_list_ = new SDPListHead(search);
        //response_list_->set_free_func((sdp_free_func_t)sdp_record_free);
    }

    // Pull off the next record from the linked list
    if (sdp_list_t* record_elem = response_list_->next()) {
        // copy out the service record
        sdp_record_t *service_record = (sdp_record_t*) record_elem->data;
        return service_record;
    }

    // fell off the end of the linked list

    // reset data structs
    delete response_list_;
    response_list_ = NULL;

    // nothing to return
    return NULL;
}

bool
BluetoothServiceDiscoveryClient::
is_dtn_router(bdaddr_t addr)
{
    bacpy(&remote_addr_,&addr);
    int is_dtn_host = 0;

    // walk through the service records on the remote host
    while (sdp_record_t *record = get_next_service_record()) {

        // fetch the list of protocol sequences
        sdp_list_t *proto_list;

        // success returns 0
        if (sdp_get_access_protos(record,&proto_list) == 0) {

            sdp_list_t* proto_seq_iter = proto_list;
            // Iterate over the elements of proto_seq_list
            // Each element's data pointer is a linked list of protocols
            while (proto_seq_iter) {

                sdp_list_t* ps_list_iter = (sdp_list_t*)proto_seq_iter->data;
                // Iterate over the elements of ps_list (via ps_list_head)
                // Each element's data pointer is a linked list
                // of type sdp_data_t* (attributes for each protocol)
                while (ps_list_iter) {

                    sdp_data_t* attr_list_iter =
                        (sdp_data_t*) ps_list_iter->data;
                    int proto = 0;

                    while (attr_list_iter) {

                        switch( attr_list_iter->dtd ) {
                            case SDP_UUID16:
                            case SDP_UUID32:
                            case SDP_UUID128:
                                proto = sdp_uuid_to_proto(
                                            &attr_list_iter->val.uuid);
                                if (proto == RFCOMM_UUID) {
                                    is_dtn_host++;
                                }
                                break; 
                        } // switch

                        sdp_data_t* ali_next = attr_list_iter->next;
                        // free each element after visiting
                        sdp_data_free(attr_list_iter);
                        // advance to next element in linked list
                        attr_list_iter = ali_next;

                    } // attr_list_iter

                    sdp_list_t* pli_next = ps_list_iter->next;
                    // free each element after visiting
                    free(ps_list_iter);
                    // advance to next element in linked list
                    ps_list_iter = pli_next;

                } // ps_list_iter

                sdp_list_t* psi_next = proto_seq_iter->next;
                // free each element after visiting
                free(proto_seq_iter);
                // advance to next element in linked list
                proto_seq_iter = psi_next;

            } // proto_seq_iter 
        } else {
            log_debug("Failed to retrieve list of protocol sequences: "
                      "%s (%d)\n", strerror(errno),errno);
            return false;
        } // sdp_get_access_protos

    } // service record
 
    return (is_dtn_host > 0);
}

BluetoothServiceRegistration::
BluetoothServiceRegistration(bdaddr_t* local, const char* logpath) :
    Logger("BluetoothServiceRegistration",logpath),
    session_handle_(NULL)
{
    bacpy(&local_addr_,local);
    status_ = register_service();
}

BluetoothServiceRegistration::
~BluetoothServiceRegistration()
{
    if (session_handle_) 
        sdp_close(session_handle_);
}

bool
BluetoothServiceRegistration::
register_service()
{
    uint32_t service_uuid_int[] = OASYS_BLUETOOTH_SDP_UUID;
    const char *service_name    = OASYS_BLUETOOTH_SDP_NAME;
    const char *service_dsc     = OASYS_BLUETOOTH_SDP_DESC;
    const char *service_prov    = OASYS_BLUETOOTH_SDP_PROV;

    uuid_t root_uuid,
           l2cap_uuid,
           rfcomm_uuid,
           svc_uuid;
    sdp_list_t *l2cap_list = 0,
               *rfcomm_list = 0,
               *root_list = 0,
               *proto_list = 0,
               *access_proto_list = 0;
    int err = 0;

    sdp_record_t *record = sdp_record_alloc();

    // set the general service ID
    sdp_uuid128_create(&svc_uuid,&service_uuid_int);
    sdp_set_service_id(record,svc_uuid);

    // make the service record publicly browsable
    sdp_uuid16_create(&root_uuid,PUBLIC_BROWSE_GROUP);
    root_list = sdp_list_append(0,&root_uuid);
    sdp_set_browse_groups(record,root_list);

    // set l2cap information
    sdp_uuid16_create(&l2cap_uuid,L2CAP_UUID);
    l2cap_list = sdp_list_append(0,&l2cap_uuid);
    proto_list = sdp_list_append(0,l2cap_list);

    // set rfcomm information
    sdp_uuid16_create(&rfcomm_uuid,RFCOMM_UUID);
    rfcomm_list = sdp_list_append(0,&rfcomm_uuid);
    sdp_list_append(proto_list,rfcomm_list);

    // attach protocol information to service record
    access_proto_list = sdp_list_append(0,proto_list);
    sdp_set_access_protos(record,access_proto_list);

    // set the name, provider, and description
    sdp_set_info_attr(record,service_name,service_prov,service_dsc);

    // connect to the local SDP server, register the service record, and
    // disconnect
    session_handle_ = sdp_connect(
                        BDADDR_ANY,   /* bind to any adapter */
                        BDADDR_LOCAL, /* connect to local server */
                        SDP_RETRY_IF_BUSY);
    err = sdp_record_register(session_handle_,record,0);

    // cleanup
    sdp_list_free(l2cap_list,0);
    sdp_list_free(rfcomm_list,0);
    sdp_list_free(root_list,0);
    sdp_list_free(proto_list,0);
    sdp_list_free(access_proto_list,0);
    sdp_record_free(record);

    return (err == 0);
}

} // namespace oasys
#endif // OASYS_BLUETOOTH_ENABLED
