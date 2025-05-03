#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

const uint8_t STTS751_TEMP_HIGH_REG = 0x00;
const uint8_t STTS751_TEMP_LOW_REG  = 0x02;
const uint8_t LPS22HH_PRES_OUT_H    = 0x2A;
const uint8_t LPS22HH_PRES_OUT_L    = 0x29;
const uint8_t LPS22HH_PRES_OUT_XL   = 0x28;
const uint8_t LPS22HH_CTRL_REG1     = 0x10;
// const uint8_t LPS22HH_CTRL_REG2     = 0x11;
// const uint8_t LPS22HH_CTRL_REG3     = 0x12;


float calc_stts751_temp(uint8_t byte_high, uint8_t byte_low) {
    int temp = ((int)byte_high * 256 + ((int)byte_low & 0xF0)) / 16;
    if(temp > 2047) {
        temp -= 4096;
    }
    // Convert to engineering units 
    double cTemp = temp * 0.0625;
    double fTemp = cTemp * 1.8 + 32;
    // Print reading to console  
    // printk("Temperature in Celsius : %.2f C \n", cTemp);
    // printk("Temperature in Fahrenheit : %.2f F \n", fTemp);
    // printk("Temperature (High Byte: %x, Low Byte: %x \n", byte_high, byte_low);

    return cTemp;
};

float calc_lps22hh_pres(uint8_t byte_high, uint8_t byte_mid, uint8_t byte_low) {
    int32_t pressure_raw = ((int32_t)byte_high << 16) | ((int32_t)byte_mid << 8) | byte_low;

    // Handle sign extension for negative pressure values
    if (pressure_raw & 0x800000) {
        pressure_raw |= 0xFF000000; // Extend the sign bits
    }

    // Divide by sensitivity to get pressure in hPa
    float pressure_hpa = (float)pressure_raw / 4096.0;

    // printk("Pressure = %.2f Pascals \n", pressure_hpa);
    // printk("Pressure (High Byte: %x, Mid Byte: %x, Low Byte: %x \n", byte_high, byte_mid, byte_low);

    return pressure_hpa;   
};

float poll_lps22hh_pres(void) {
    const struct i2c_dt_spec dt_spec_lps22hh = I2C_DT_SPEC_GET(DT_NODELABEL(my_lps22hh));
    uint8_t high_byte, mid_byte, low_byte;
    int err;

    if (i2c_is_ready_dt(&dt_spec_lps22hh) != true) {
        printk("I2C0 bus not ready for some reason!");
        return -1E10;
    }
    uint8_t config[2] = {LPS22HH_CTRL_REG1,0x10};
    err = i2c_write_dt(&dt_spec_lps22hh, config, sizeof(config));

    err = i2c_reg_read_byte_dt(&dt_spec_lps22hh, LPS22HH_PRES_OUT_XL, &low_byte);
    err = i2c_reg_read_byte_dt(&dt_spec_lps22hh, LPS22HH_PRES_OUT_L, &mid_byte);
    err = i2c_reg_read_byte_dt(&dt_spec_lps22hh, LPS22HH_PRES_OUT_H, &high_byte);
    
    float Pa_value = calc_lps22hh_pres(high_byte, mid_byte, low_byte);

    printk("Pressure = %.2f Pascals \n", Pa_value);

    return Pa_value;
};

float poll_stts751_temp(void) {
    const struct i2c_dt_spec dt_spec_stts751 = I2C_DT_SPEC_GET(DT_NODELABEL(my_stts751));
    uint8_t low_byte, high_byte;
    int err;

    if (i2c_is_ready_dt(&dt_spec_stts751) != true) {
        printk("I2C0 bus not ready for some reason!");
        return -1E10;
    }

    err = i2c_reg_read_byte_dt(&dt_spec_stts751, STTS751_TEMP_LOW_REG, &low_byte);

    err = i2c_reg_read_byte_dt(&dt_spec_stts751, STTS751_TEMP_HIGH_REG, &high_byte);

    double tempC = calc_stts751_temp(high_byte, low_byte);

    printk("Temperature in Celsius : %.2f C \n", tempC);

    return tempC;

};
