/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <inttypes.h>
#include <string.h>

#include <bluetooth/bonding_sync.h>

#include "syscfg/syscfg.h"
#include "host/ble_hs.h"

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static struct ble_store_value_sec
    ble_store_pebble_our_secs[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
#endif

static int ble_store_pebble_num_our_secs;

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static struct ble_store_value_sec
    ble_store_pebble_peer_secs[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
#endif

static int ble_store_pebble_num_peer_secs;

#define BLE_FLAG_AUTHENTICATED      0x01
#define BLE_FLAG_SECURE_CONNECTIONS 0x02

/*****************************************************************************
 * $sec                                                                      *
 *****************************************************************************/

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static void
ble_store_pebble_print_value_sec(const struct ble_store_value_sec *sec)
{
    if (sec->ltk_present) {
        BLE_HS_LOG(INFO, "ediv=%u rand=%llu authenticated=%d ltk=",
                       sec->ediv, sec->rand_num, sec->authenticated);
        ble_hs_log_flat_buf(sec->ltk, 16);
        BLE_HS_LOG(INFO, " ");
    }
    if (sec->irk_present) {
        BLE_HS_LOG(INFO, "irk=");
        ble_hs_log_flat_buf(sec->irk, 16);
        BLE_HS_LOG(INFO, " ");
    }
    if (sec->csrk_present) {
        BLE_HS_LOG(INFO, "csrk=");
        ble_hs_log_flat_buf(sec->csrk, 16);
        BLE_HS_LOG(INFO, " ");
    }

    BLE_HS_LOG(INFO, "\n");
}
#endif

static void
ble_store_pebble_print_key_sec(const struct ble_store_key_sec *key_sec)
{
    if (ble_addr_cmp(&key_sec->peer_addr, BLE_ADDR_ANY)) {
        BLE_HS_LOG(INFO, "peer_addr_type=%d peer_addr=",
                       key_sec->peer_addr.type);
        ble_hs_log_flat_buf(key_sec->peer_addr.val, 6);
        BLE_HS_LOG(INFO, " ");
    }
}

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static int
ble_store_pebble_find_sec(const struct ble_store_key_sec *key_sec,
                       const struct ble_store_value_sec *value_secs,
                       int num_value_secs)
{
    const struct ble_store_value_sec *cur;
    int i;

    if (!ble_addr_cmp(&key_sec->peer_addr, BLE_ADDR_ANY)) {
        if (key_sec->idx < num_value_secs) {
            return key_sec->idx;
        }
    } else if (key_sec->idx == 0) {
        for (i = 0; i < num_value_secs; i++) {
            cur = &value_secs[i];

            if (!ble_addr_cmp(&cur->peer_addr, &key_sec->peer_addr)) {
                return i;
            }
        }
    }

    return -1;
}
#endif

static int
ble_store_pebble_read_our_sec(const struct ble_store_key_sec *key_sec,
                           struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int idx;

    idx = ble_store_pebble_find_sec(key_sec, ble_store_pebble_our_secs,
                                 ble_store_pebble_num_our_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_sec = ble_store_pebble_our_secs[idx];
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_pebble_write_our_sec(const struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    struct ble_store_key_sec key_sec;
    int idx;

    BLE_HS_LOG(INFO, "persisting our sec; ");
    ble_store_pebble_print_value_sec(value_sec);

    ble_store_key_from_value_sec(&key_sec, value_sec);
    idx = ble_store_pebble_find_sec(&key_sec, ble_store_pebble_our_secs,
                                 ble_store_pebble_num_our_secs);
    if (idx == -1) {
        if (ble_store_pebble_num_our_secs >= MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
            BLE_HS_LOG(ERROR, "error persisting our sec; too many entries "
                              "(%d)\n", ble_store_pebble_num_our_secs);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_pebble_num_our_secs;
        ble_store_pebble_num_our_secs++;
    }

    ble_store_pebble_our_secs[idx] = *value_sec;

    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static int
ble_store_pebble_delete_obj(void *values, int value_size, int idx,
                         int *num_values)
{
    uint8_t *dst;
    uint8_t *src;
    int move_count;

    (*num_values)--;
    if (idx < *num_values) {
        dst = values;
        dst += idx * value_size;
        src = dst + value_size;

        move_count = *num_values - idx;
        memmove(dst, src, move_count);
    }

    return 0;
}

static int
ble_store_pebble_delete_sec(const struct ble_store_key_sec *key_sec,
                         struct ble_store_value_sec *value_secs,
                         int *num_value_secs)
{
    int idx;
    int rc;

    idx = ble_store_pebble_find_sec(key_sec, value_secs, *num_value_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    rc = ble_store_pebble_delete_obj(value_secs, sizeof *value_secs, idx,
                                  num_value_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
#endif

static int
ble_store_pebble_delete_our_sec(const struct ble_store_key_sec *key_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int rc;

    rc = ble_store_pebble_delete_sec(key_sec, ble_store_pebble_our_secs,
                                  &ble_store_pebble_num_our_secs);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_pebble_delete_peer_sec(const struct ble_store_key_sec *key_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int rc;

    rc = ble_store_pebble_delete_sec(key_sec, ble_store_pebble_peer_secs,
                                  &ble_store_pebble_num_peer_secs);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_pebble_read_peer_sec(const struct ble_store_key_sec *key_sec,
                            struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int idx;

    idx = ble_store_pebble_find_sec(key_sec, ble_store_pebble_peer_secs,
                             ble_store_pebble_num_peer_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_sec = ble_store_pebble_peer_secs[idx];
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_pebble_write_peer_sec(const struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    struct ble_store_key_sec key_sec;
    int idx;

    BLE_HS_LOG(INFO, "persisting peer sec; ");
    ble_store_pebble_print_value_sec(value_sec);

    ble_store_key_from_value_sec(&key_sec, value_sec);
    idx = ble_store_pebble_find_sec(&key_sec, ble_store_pebble_peer_secs,
                                 ble_store_pebble_num_peer_secs);
    if (idx == -1) {
        if (ble_store_pebble_num_peer_secs >= MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
            BLE_HS_LOG(ERROR, "error persisting peer sec; too many entries "
                             "(%d)\n", ble_store_pebble_num_peer_secs);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_pebble_num_peer_secs;
        ble_store_pebble_num_peer_secs++;
    }

    ble_store_pebble_peer_secs[idx] = *value_sec;

    BleBonding bonding = { 0 };
    BTDeviceAddress addr = { 0 };
    
    bonding.is_gateway = true;
    
    if (value_sec->ltk_present) {
        bonding.pairing_info.is_remote_encryption_info_valid = true;
        bonding.pairing_info.remote_encryption_info.ediv = value_sec->ediv;
        bonding.pairing_info.remote_encryption_info.rand = value_sec->rand_num;
        memcpy(bonding.pairing_info.remote_encryption_info.ltk.data, value_sec->ltk, 16);
    }
    
    if (value_sec->irk_present) {
        bonding.pairing_info.is_remote_identity_info_valid = true;
        memcpy(bonding.pairing_info.irk.data, value_sec->irk, 16);
    }

    if (value_sec->csrk_present) {
        bonding.pairing_info.is_remote_signing_info_valid = true;
        memcpy(bonding.pairing_info.csrk.data, value_sec->csrk, 16);
    }

    if (value_sec->authenticated) {
        bonding.flags |= BLE_FLAG_AUTHENTICATED;
    }

    if (value_sec->sc) {
        bonding.flags |= BLE_FLAG_SECURE_CONNECTIONS;
    }

    bonding.pairing_info.identity.is_random_address = value_sec->peer_addr.type == BLE_ADDR_RANDOM;
    memcpy(bonding.pairing_info.identity.address.octets, value_sec->peer_addr.val, 6);

    memcpy(addr.octets, value_sec->peer_addr.val, 6);

    bt_driver_cb_handle_create_bonding(&bonding, &addr);

    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

/*****************************************************************************
 * $api                                                                      *
 *****************************************************************************/

/**
 * Searches the database for an object matching the specified criteria.
 *
 * @return                      0 if a key was found; else BLE_HS_ENOENT.
 */
int
ble_store_pebble_read(int obj_type, const union ble_store_key *key,
                   union ble_store_value *value)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        /* An encryption procedure (bonding) is being attempted.  The nimble
         * stack is asking us to look in our key database for a long-term key
         * corresponding to the specified ediv and random number.
         *
         * Perform a key lookup and populate the context object with the
         * result.  The nimble stack will use this key if this function returns
         * success.
         */
        BLE_HS_LOG(INFO, "looking up peer sec; ");
        ble_store_pebble_print_key_sec(&key->sec);
        BLE_HS_LOG(INFO, "\n");
        rc = ble_store_pebble_read_peer_sec(&key->sec, &value->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        BLE_HS_LOG(INFO, "looking up our sec; ");
        ble_store_pebble_print_key_sec(&key->sec);
        BLE_HS_LOG(INFO, "\n");
        rc = ble_store_pebble_read_our_sec(&key->sec, &value->sec);
        return rc;

    default:
        return BLE_HS_ENOTSUP;
    }
}

/**
 * Adds the specified object to the database.
 *
 * @return                      0 on success; BLE_HS_ESTORE_CAP if the database
 *                                  is full.
 */
int
ble_store_pebble_write(int obj_type, const union ble_store_value *val)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        rc = ble_store_pebble_write_peer_sec(&val->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        rc = ble_store_pebble_write_our_sec(&val->sec);
        return rc;

    default:
        return BLE_HS_ENOTSUP;
    }
}

int
ble_store_pebble_delete(int obj_type, const union ble_store_key *key)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        rc = ble_store_pebble_delete_peer_sec(&key->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        rc = ble_store_pebble_delete_our_sec(&key->sec);
        return rc;

    default:
        return BLE_HS_ENOTSUP;
    }
}

void
ble_store_pebble_init(void)
{
    ble_hs_cfg.store_read_cb = ble_store_pebble_read;
    ble_hs_cfg.store_write_cb = ble_store_pebble_write;
    ble_hs_cfg.store_delete_cb = ble_store_pebble_delete;
}

void ble_store_pebble_add_bonding(const BleBonding *bonding)
{
    int idx;
    struct ble_store_value_sec value_sec = { 0 };
    struct ble_store_key_sec key_sec;

    value_sec.key_size = 16;

    if (bonding->pairing_info.is_remote_encryption_info_valid) {
        value_sec.ediv = bonding->pairing_info.remote_encryption_info.ediv;
        value_sec.rand_num = bonding->pairing_info.remote_encryption_info.rand;
        value_sec.ltk_present = true;
        memcpy(value_sec.ltk, bonding->pairing_info.remote_encryption_info.ltk.data, 16);
    }

    if (bonding->pairing_info.is_remote_identity_info_valid) {
        value_sec.irk_present = true;
        memcpy(value_sec.irk, bonding->pairing_info.irk.data, 16);
    }

    if (bonding->pairing_info.is_remote_signing_info_valid) {
        value_sec.csrk_present = true;
        memcpy(value_sec.csrk, bonding->pairing_info.csrk.data, 16);
    }

    value_sec.authenticated = !!(bonding->flags & BLE_FLAG_AUTHENTICATED);
    value_sec.sc = !!(bonding->flags & BLE_FLAG_SECURE_CONNECTIONS);

    value_sec.peer_addr.type = bonding->pairing_info.identity.is_random_address ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
    memcpy(value_sec.peer_addr.val, bonding->pairing_info.identity.address.octets, 6);

    ble_store_key_from_value_sec(&key_sec, &value_sec);
    idx = ble_store_pebble_find_sec(&key_sec, ble_store_pebble_peer_secs,
                                    ble_store_pebble_num_peer_secs);
    if (idx == -1) {
        if (ble_store_pebble_num_peer_secs >= MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
            BLE_HS_LOG(ERROR, "error persisting peer sec; too many entries "
                             "(%d)\n", ble_store_pebble_num_peer_secs);
            return;
        }

        idx = ble_store_pebble_num_peer_secs;
        ble_store_pebble_num_peer_secs++;
    }

    ble_store_pebble_peer_secs[idx] = value_sec;
}

void ble_store_pebble_remove_bonding(const BleBonding *bonding)
{
    struct ble_store_key_sec key_sec = { 0 };

    key_sec.peer_addr.type = bonding->pairing_info.identity.is_random_address ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
    memcpy(key_sec.peer_addr.val, bonding->pairing_info.identity.address.octets, 6);

    ble_store_pebble_delete_peer_sec(&key_sec);
}