#include <minix/drivers.h>
#include <minix/driver.h>
#include <minix/ds.h>
#include <sys/ioctl.h>
/* #include <minix/ioctl.h> */
#include <sys/ucred.h>  /* Needed for getnucred */
#include <minix/ipc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "secret.h"

#define SSGRANT _IOW('K', 1, uid_t)
/* Secret storage */
PRIVATE char secret_buffer[SECRET_SIZE];
PRIVATE int secret_length = 0;
PRIVATE uid_t secret_owner = -1;
PRIVATE int read_open_count = 0;

/* Function prototypes */
PRIVATE char * secret_name(void);
PRIVATE int secret_open(struct driver *d, message *m);
PRIVATE int secret_close(struct driver *d, message *m);
PRIVATE int secret_transfer(int proc_nr, int opcode, 
    u64_t position, iovec_t *iov, unsigned nr_req);
PRIVATE int secret_ioctl(struct driver *d, message *m);
PRIVATE int sef_cb_init(int type, sef_init_info_t *info);
PRIVATE int sef_cb_lu_state_save(int state);
PRIVATE void sef_local_startup(void);

PRIVATE void secret_geometry(struct partition *entry);

PRIVATE struct device * secret_prepare(int dev);

/* Device driver structure */
PRIVATE struct driver secret_tab =
{
    secret_name,      /* Device name */
    secret_open,      /* Open function */
    secret_close,     /* Close function */
    secret_ioctl,     /* IOCTL function */
    secret_prepare,   /* Prepare function */
    secret_transfer,  /* Transfer function */
    nop_cleanup,      /* Cleanup function (not used, so set to nop) */
    secret_geometry,     /* Geometry function (not needed, so set to nop) */
    nop_alarm,        /* Alarm function (not used) */
    nop_cancel,       /* Cancel function (not used) */
    nop_select,       /* Select function (not used) */
    nop_ioctl,        /* Second IOCTL function (not used) */
    do_nop,           /* Default case for any unhandled operations */
};


PRIVATE char * secret_name(void)
{
    printf("secret_name()\n");
    return "secret";
}

/* Open function */
PRIVATE int secret_open(struct driver *d, message *m) {
    int mode = m->COUNT;
    struct ucred cred;
    int res;

    /* Get the process owner */
    res = getnucred(m->IO_ENDPT, &cred);
    if (res != OK) return res;

    if (secret_owner == -1) {
        /* No owner: Take ownership */
        secret_owner = cred.uid;
    } else if (cred.uid != secret_owner && (mode & R_BIT)) {
        /* Not the owner trying to read */
        return EACCES;
    } else if (mode == (R_BIT | W_BIT)) {
        /* O_RDWR is not allowed */
        return EACCES;
    } else if ((mode & W_BIT) && secret_length > 0) {
        /* Already full, can't write */
        return ENOSPC;
    }

    if (mode & R_BIT) {
        read_open_count++;
    }

    return OK;
}

/* Close function */
PRIVATE int secret_close(struct driver *d, message *m) {
    struct ucred cred;
    int res;

    /* Get process owner */
    res = getnucred(m->IO_ENDPT, &cred);
    if (res != OK) return res;

    /* If closing a read fd, decrement counter */
    if (cred.uid == secret_owner && read_open_count > 0) {
        read_open_count--;
        if (read_open_count == 0) {
            /* Reset secret when the last read fd closes */
            secret_length = 0;
            secret_owner = -1;
            memset(secret_buffer, 0, SECRET_SIZE);
        }
    }
    return OK;
}

/* Write (transfer) function */
PRIVATE int secret_transfer(int proc_nr, int opcode, u64_t position, iovec_t *iov, unsigned nr_req) {
    int bytes, res;
    
    if (opcode == DEV_GATHER_S) { /* Read */
        bytes = (secret_length - position.lo < iov->iov_size) ? (secret_length - position.lo) : iov->iov_size;
        if (bytes <= 0) return OK;

        res = sys_safecopyto(proc_nr, iov->iov_addr, 0, (vir_bytes)(secret_buffer + position.lo), bytes, D);
        if (res != OK) return res;
        iov->iov_size -= bytes;
    } else if (opcode == DEV_SCATTER_S) { /* Write */
        if (secret_length > 0) return ENOSPC; /* Prevent multiple writes */

        bytes = (iov->iov_size > SECRET_SIZE) ? SECRET_SIZE : iov->iov_size;
        res = sys_safecopyfrom(proc_nr, iov->iov_addr, 0, (vir_bytes)secret_buffer, bytes, D);
        if (res != OK) return res;

        secret_length = bytes;
    } else {
        return EINVAL;
    }

    return OK;
}

/* IOCTL handler */
PRIVATE int secret_ioctl(struct driver *d, message *m) {
    uid_t grantee;
    int res;

    if (m->REQUEST == SSGRANT) {
        if (secret_owner == -1) return ENOENT; /* No secret stored */

        /* Get grantee ID */
        res = sys_safecopyfrom(m->IO_ENDPT, (vir_bytes)m->IO_GRANT, 0, (vir_bytes)&grantee, sizeof(grantee), D);
        if (res != OK) return res;

        /* Update ownership */
        secret_owner = grantee;
        return OK;
    }

    return ENOTTY; /* Unsupported IOCTL */
}

/* Prepare function */
PRIVATE struct device * secret_prepare(int dev) {
    static struct device secret_device;
    secret_device.dv_size.lo = secret_length;
    secret_device.dv_size.hi = 0;
    return &secret_device;
}

/* SEF initialization */
PRIVATE void sef_local_startup(void) {
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    sef_setcb_lu_state_save(sef_cb_lu_state_save);
    sef_startup();
}

/* SEF state save */
PRIVATE int sef_cb_lu_state_save(int state) {
    ds_publish_mem("secret_buffer", secret_buffer, SECRET_SIZE, DSF_OVERWRITE);
    ds_publish_u32("secret_length", secret_length, DSF_OVERWRITE);
    ds_publish_u32("secret_owner", secret_owner, DSF_OVERWRITE);
    return OK;
}

/* SEF state restore */
PRIVATE int sef_cb_init(int type, sef_init_info_t *info) {
    u32_t temp_len;
    u32_t temp_owner;
    if (type == SEF_INIT_LU) {
        size_t len;
        ds_retrieve_mem("secret_buffer", secret_buffer, &len);
        ds_retrieve_u32("secret_length", &temp_len);
        ds_retrieve_u32("secret_owner", &temp_owner);  
        secret_length = (int) temp_len;
        secret_owner = (uid_t) temp_owner;
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
PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    printf("secret_geometry()\n");
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

/* Main */
PUBLIC int main(int argc, char **argv) {
    sef_local_startup();
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}
