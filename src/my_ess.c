#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "my_ess.h"

LOG_MODULE_DECLARE(sample_bluey);

static void myess_ccc_temp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    // if the Central (client) wants to be notified, it will change to BT_GATT_CCC_NOTIFY
    // otherwise, it will be 0
    LOG_INF("CCC changed for Temp Char to: %d\n", value);
}

/* Environmental Sensing Service Declaration */
BT_GATT_SERVICE_DEFINE(
    my_es_svc,  // [0]
    BT_GATT_PRIMARY_SERVICE(BT_UUID_MY_ESS),  // [1]

    BT_GATT_CHARACTERISTIC(BT_UUID_MY_ESS_TEMP,  // [2]
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),  // [3]
    
    BT_GATT_CCC(myess_ccc_temp_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),  // [4]
);


int my_ess_send_sensor_notify(uint32_t sensor_value)
{
    int err; 
    err = bt_gatt_notify(
        NULL,  // notify all connections currently connected to this peripheral device
        &my_es_svc.attrs[2],   // because service declaration is handle 0x0001, ours is 0x0002
        &sensor_value, 
        sizeof(sensor_value));
    return err;
}
