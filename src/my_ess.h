#ifndef MY_ESS_H_
#define MY_ESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
// #include <zephyr/bluetooth/uuid.h>


// 0x181A from Bluetooth official assigned numbers for the Environmental Sensing service
#define BT_UUID_MY_ESS_VAL  BT_UUID_16_ENCODE(0x181A)
#define BT_UUID_MY_ESS  BT_UUID_DECLARE_16(0x181A) // NOTE: BT_UUID_DECLARE_16 doesn't take little-endian


// 0x2A6E from Bluetooth official assigned numbers for Temperature characteristic
#define BT_UUID_MY_ESS_TEMP_VAL  BT_UUID_16_ENCODE(0x2A6E)
#define BT_UUID_MY_ESS_TEMP  BT_UUID_DECLARE_16(0x2A6E)  // NOTE: BT_UUID_DECLARE_16 doesn't take little-endian


int my_ess_send_sensor_notify(uint32_t sensor_value);


#ifdef __cplusplus
}
#endif

#endif  /* MY_ESS_H_ */