#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#include <math.h>
#include "main.h"
#include "my_ess.h"

#define SLEEP_TIME_MS   1000
#define STACKSIZE 2048

#define THREAD0_PRIORITY 7 //ble_notify thread
#define THREAD1_PRIORITY 7 //stss751_poll thread
#define THREAD2_PRIORITY 7 //lps22hh_poll thread

/* Define the I2C slave device address and the addresses of relevant registers */
#define STTS751_TEMP_HIGH_REG            0x00
#define STTS751_TEMP_LOW_REG             0x02
// #define STTS751_CONFIG_REG               0x03
#define LPS22HH_PRES_OUT_H               0x2A
#define LPS22HH_PRES_OUT_L               0x29
#define LPS22HH_PRES_OUT_XL              0x28
#define LPS22HH_CTRL_REG1                0x10
// #define LPS22HH_CTRL_REG2                0x11
// #define LPS22HH_CTRL_REG3                0x12

/* Get the node identifier of the sensor */
#define I2C_NODE    DT_NODELABEL(my_stts751)
#define I2C_NODE1   DT_NODELABEL(my_lps22hh)

BUILD_ASSERT(DT_NODE_HAS_STATUS(I2C_NODE, okay), "I2C_NODE not okay");
BUILD_ASSERT(DT_NODE_HAS_STATUS(I2C_NODE1, okay), "I2C_NODE1 not okay");


LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

typedef struct {
    float temp_value;  // assume sensors no more precise than float32
    float pres_value;
    uint8_t sensor_id;  // up to 256 sensors
} SensorReading;

K_MUTEX_DEFINE(msgq_mutex);

K_MSGQ_DEFINE(msgq_ble_notify, sizeof(SensorReading), 16, 8);

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_CONN;


/*static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    //(
    //    BT_LE_ADV_OPT_CONNECTABLE
    //), /* Connectable advertising */
    //800, /* Min Advertising Interval 500ms (800*0.625ms) */
    //1001, /* Max Advertising Interval 625.625ms (1001*0.625ms) */
    //NULL); /* Set to NULL for undirected advertising */


static const struct bt_data adv_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, 
        CONFIG_BT_DEVICE_NAME, 
        sizeof(CONFIG_BT_DEVICE_NAME) - 1
    ),

};

static const struct bt_data scan_data[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_16_ENCODE(BT_UUID_ESS_VAL)),
};

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
		//printk("Connection failed (err %u)\n", err);
        LOG_ERR("Connection failed (err %u)\n", err);
		return;
	}
	//printk("Connected\n");
    LOG_INF("Connected\n");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	// printk("Disconnected (reason %u)\n", reason);
    LOG_INF("Disconnected (reason %u)\n", reason);
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

BT_GATT_SERVICE_DEFINE(
    my_ess_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),  // [0]
    BT_GATT_CHARACTERISTIC(   // [1]
        BT_UUID_TEMPERATURE,  // [2]
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL  
    ),
    
    BT_GATT_CCC(    // [3]
        NULL,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE
    ),
);

BT_GATT_SERVICE_DEFINE(
    my_ess_service1,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),  // [0]
    BT_GATT_CHARACTERISTIC(   // [1]
        BT_UUID_PRESSURE,  // [2]
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL  
    ),
    
    BT_GATT_CCC(    // [3]
        NULL,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE
    ),
);

void thread0(void) {
    // printk("Started notifying thread. \n");
    LOG_DBG("Started notifying thread.");
    int16_t ble_char_temperature;
    int32_t ble_char_pressure;
    while (true) {
        SensorReading now_sensor_reading;
        // k_mutex_lock(&msgq_mutex, K_FOREVER);
        k_msgq_get(&msgq_ble_notify, &now_sensor_reading, K_FOREVER);
        // k_mutex_unlock(&msgq_mutex);

        LOG_DBG("Message Recieved %d", now_sensor_reading.sensor_id);

        if (now_sensor_reading.sensor_id == 0) // this is the temperature sensor
        {
            LOG_DBG("Message Recieved %f", (double)now_sensor_reading.temp_value);
            // according to Bluetooth Characteristic Specs, raw value is divided by 100
            ble_char_temperature = (int16_t)(nearbyintf(now_sensor_reading.temp_value * 100));
            bt_gatt_notify(  // actual call to notify and change the [2] value
                NULL,
                &my_ess_service.attrs[2],
                &(ble_char_temperature),
                sizeof(ble_char_temperature)
            );
        }

        if (now_sensor_reading.sensor_id == 1) // this is the pressure sensor
        {
            LOG_DBG("Message Recieved %f", (double)now_sensor_reading.pres_value);
            // according to Bluetooth Characteristic Specs, raw value is divided by ??
            ble_char_pressure = (int32_t)(nearbyintf(now_sensor_reading.pres_value));
            bt_gatt_notify(  // actual call to notify and change the [2] value
                NULL,
                &my_ess_service1.attrs[2],
                &(ble_char_pressure),
                sizeof(ble_char_pressure)
            );
        }
    }
};
void thread1(void* void_msec_period) {
    float temp_degC;
    SensorReading now_stts751_reading;
    // printk("Started STTS751 polling thread. \n");
    LOG_DBG("Started STTS751 polling thread.");
    while (true) {
        temp_degC = poll_stts751_temp();
        now_stts751_reading.sensor_id = 0;
        now_stts751_reading.temp_value = temp_degC;
        // k_mutex_lock(&msgq_mutex, K_FOREVER);
        k_msgq_put(&msgq_ble_notify, &now_stts751_reading, K_FOREVER);
        // k_mutex_unlock(&msgq_mutex);
        k_msleep(SLEEP_TIME_MS);
    }
};
void thread2(void) {
    float Pa_value;
    SensorReading now_lps22hh_reading;
    // printk("Started LPS22HH polling thread. \n");
    LOG_DBG("Started LPS22HH polling thread.");
    while (true) {
        Pa_value = poll_lps22hh_pres();
        now_lps22hh_reading.sensor_id = 1;
        now_lps22hh_reading.pres_value = Pa_value;
        // k_mutex_lock(&msgq_mutex, K_FOREVER);
        k_msgq_put(&msgq_ble_notify, &now_lps22hh_reading, K_FOREVER);
        // k_mutex_unlock(&msgq_mutex);
        k_msleep(2 * SLEEP_TIME_MS);
    }
};

/* Define and initialize the threads */
K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL,
		THREAD1_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread2_id, STACKSIZE, thread2, NULL, NULL, NULL,
		THREAD2_PRIORITY, 0, 0);

int main(void)
{
	int err;

    // k_mutex_init(&msgq_mutex);

    err = bt_enable(NULL);
    if (err) {
        // printk("Bluetooth init failed (err %d)\n", err);
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    bt_conn_cb_register(&connection_callbacks);
    // printk("Bluetooth initialized\n");
    LOG_INF("Bluetooth initialized\n");

    err = bt_le_adv_start(
        adv_param,
        adv_data, ARRAY_SIZE(adv_data),
        scan_data, ARRAY_SIZE(scan_data)
    );
    

    if (err) {
        // printk("Advertising failed to start (err %d)\n", err);
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return err;
    }
    // printk("Advertising successfully started\n");
    LOG_INF("Advertising successfully started\n");
    
    while (true) {
        // printk("current time is %d \n \n", k_uptime_get_32());
        LOG_INF("current time is %d\n", k_uptime_get_32());
        poll_stts751_temp();    // Read and print temperature
        poll_lps22hh_pres();    // Read and print pressure
        k_msleep(2 * SLEEP_TIME_MS);
    }
}
