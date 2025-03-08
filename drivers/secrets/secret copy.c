#include <minix/drivers.h>
#include <minix/driver.h>
#include <minix/ds.h>
#include <sys/ioctl.h>
#include <minix/ioctl.h>
#include <sys/ioc_secret.h>
#include <sys/ucred.h>
#include <minix/ipc.h>
#include <stdio.h>
#include <stdlib.h>

#include "secret.h"

/*
 * Function prototypes for the secret driver.
 */
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * secret_prepare, (int device) );
FORWARD _PROTOTYPE( int secret_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void secret_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the secret driver. */
PRIVATE struct driver secret_tab =
{
    secret_name,
    secret_open,
    secret_close,
    nop_ioctl,
    secret_prepare,
    secret_transfer,
    nop_cleanup,
    secret_geometry,
    nop_alarm,
    nop_cancel,
    nop_select,
    nop_ioctl,
    do_nop,
};

/** Represents the /dev/secret device. */
PRIVATE struct device secret_device;

PRIVATE char secret_buffer[SECRET_SIZE];
PRIVATE int secret_length = 0;
PRIVATE uid_t secret_owner = -1;
PRIVATE int read_open_cnt = 0;

/** State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;

PRIVATE int in_use = 0;

PRIVATE char * secret_name(void)
{
    printf("secret_name()\n");
    return "secret";
}

PRIVATE int secret_open(d, m){
    int mode = m->COUNT;
    struct ucred cred;
    int res;

    res = getnucred(m->IO_ENDPT, &cred);
    if (res != OK)
    {
        return res;
    }
    if (secret_owner == -1){
        //nobody owns it so take it for yourself
        secret_owner = cred.uid;
    }
    else if (cred.uid != secret_owner && (mode & R_BIT)) {
        //non owner is trying to open
        return EACCES;
    }
    else if (mode == (R_BIT | W_BIT)) {
        //read and write not allowed
        return EACCES;
    }
    else if ((mode & W_BIT) && secret_len > 0) {
        //trying to open for writing when something has already written to it
        return ENOSPC;
    }

    if (mode & R_BIT) {
        read_open_cnt++;
    }
    return OK;
}

PRIVATE int secret_close(d, m){
    struct ucred cred;
    int res;

    //get process credentials
    res = getnucred(m->IO_ENDPT, &cred);
    if (res != OK)
    {
        return res;
    }
    if (cred.uid == secret_owner && read_open_cnt > 0) {
        read_open_cnt--;

        if (read_open_cnt == 0) {
            //owner is closing the device, reset the secret
            secret_owner = -1;
            secret_len = 0;
            memset(secret_buffer, 0, SECRET_SIZE);
        }

    }
}
    

PRIVATE struct device * secret_prepare(dev)
    int dev;
{
    secret_device.dv_base.lo = 0;
    secret_device.dv_base.hi = 0;
    secret_device.dv_size.lo = strlen(secret_MESSAGE);
    secret_device.dv_size.hi = 0;
    return &secret_device;
}

PRIVATE int secret_transfer(proc_nr, opcode, position, iov, nr_req)
    int proc_nr;
    int opcode;
    u64_t position;
    iovec_t *iov;
    unsigned nr_req;
{
    int bytes, res;

    printf("secret_transfer()\n");

    // bytes = strlen(secret_MESSAGE) - position.lo < iov->iov_size ?
    //         strlen(secret_MESSAGE) - position.lo : iov->iov_size;

    // if (bytes <= 0)
    // {
    //     return OK;
    // }
    switch (opcode)
    {
        case DEV_GATHER_S: //read
            if (secret_len - position.lo < iov->iov_size) {
                bytes = secret_len - position.lo;
            }
            else {
                bytes = iov->iov_size;
            }
            if (bytes <= 0) {
                return OK;
            }
            res = sys_safecopyto(proc_nr, iov->iov_addr, 0,
                                (vir_bytes) (secret_buffer + position.lo),
                                 bytes, D);
            if (res != OK) {
                return res;
            }
            iov->iov_size -= bytes;
            break;
        
        case DEV_SCATTER_S: //write
            if (secret_len > 0){
                //cant write on top of something else
                return ENOSPC;
            }
            if (iov->iov_size > SECRET_SIZE) {
                //too big
                return ENOSPC;
            }
            res = sys_safecopyfrom(proc_nr, iov->iov_addr, 0,
                                (vir_bytes) secret_buffer,
                                iov->iov_size, D);
            if (res != OK) {
                return res;
            }
            secret_len = iov->iov_size;
            break;

        default:
            return EINVAL;
    }
    return ret;
}

PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    printf("secret_geometry()\n");
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

/* SEF state save */
PRIVATE int sef_cb_lu_state_save(int state) {
    ds_publish_mem("secret_buffer", secret_buffer, SECRET_SIZE, DSF_OVERWRITE);
    ds_publish_u32("secret_length", secret_length, DSF_OVERWRITE);
    ds_publish_u32("secret_owner", secret_owner, DSF_OVERWRITE);
    return OK;
}

PRIVATE int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    return OK;
}

PRIVATE void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

/* SEF state restore */
PRIVATE int sef_cb_init(int type, sef_init_info_t *info) {
    if (type == SEF_INIT_LU) {
        size_t len;
        ds_retrieve_mem("secret_buffer", secret_buffer, &len);
        ds_retrieve_u32("secret_length", &secret_length);
        ds_retrieve_u32("secret_owner", &secret_owner);
        ds_delete_mem("secret_buffer");
        ds_delete_u32("secret_length");
        ds_delete_u32("secret_owner");
    } else {
        memset(secret_buffer, 0, SECRET_SIZE);
        secret_length = 0;
        secret_owner = -1;
    }

    driver_announce();
    return OK;
}

PUBLIC int main(int argc, char **argv)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}

