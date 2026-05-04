#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NST175_CONVERSION_TIME 40 // From datasheet: conversion time TCONV 30-40 ms

typedef enum {
    NST175_STAT_OK,
    NST175_STAT_HANDLE_NF, // Device handle not found or identity check failed
    NST175_STAT_PLATFORM_NF, // One or more of basic platform functions not found
    NST175_STAT_ARG_INVALID, // Invalid argument provided
    NST175_STAT_I2C_FAIL, // I2C bus failure detected
    NST175_STAT_ID_NS, // Could not check device ID or its not supported
    NST175_STAT_TIMEOUT // Timeout expired for blocking operation
} nst175_status_t;

typedef enum { NST175_LIMIT_LOW, NST175_LIMIT_HIGH } nst175_limit_t;

typedef enum { NST175_THERMOSTAT_MODE_COMP, NST175_THERMOSTAT_MODE_IT } nst175_thermostat_mode_t;

typedef enum { NST175_ALARM_POLARITY_LOW, NST175_ALARM_POLARITY_HIGH } nst175_alarm_polarity_t;

typedef struct nst175_s {
    /* Platform related */
    struct {
        void *handle; // Optional handle for I2C interface
        void (*print)(const char *const fmt, ...); // Optional debug print
        bool (*read)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        bool (*write)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        void (*delay)(uint32_t ms);
    } interface;

    /* Per-device driver internals - do not modify */
    struct {
        uint32_t identity; // Device handle identity to check if `nst175_t` structure can be safely used
        nst175_status_t error; // Last detected driver error
        float lsb; // LSB weight for temperature calculation
        uint8_t address; // Device I2C slave address
        uint8_t resolution; // Resolution value (9-12)
        bool shutdown; // Shutdown mode state
    } cache;
} nst175_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize/Renitialize the NST175 device
 * @param dev pointer to the device structure
 * @param address 7bit device I2C slave address from datasheet
 * @param reset reset configuration registers to default power-up state
 * @return Exit driver status
 * @note Interface read/write/delay function pointers have to be provided prior `NST175_Init()` call.
 * @note One-shot timeout will be forced inside to `100ms` if has not been set yet
 */
nst175_status_t NST175_Init(nst175_t *dev, uint8_t address, bool reset);

/**
 * @brief Read active low-power shutdown mode setting
 * @param dev pointer to the device structure
 * @param enabled pointer to variable to keep the low-power shutdown mode setting
 * @return Exit driver status
 */
nst175_status_t NST175_ShutdownModeGet(nst175_t *dev, bool *enabled);

/**
 * @brief Write new low-power shutdown mode setting
 * @param dev pointer to the device structure
 * @param enabled `true` to enable
 * @return Exit driver status
 * @note In shutdown mode using `NST175_TemperatureGet()` will lead to one-shot command to be sent to perform a
temperature transition, and the device shuts down automatically when the temperature transition is complete
 */
nst175_status_t NST175_ShutdownModeSet(nst175_t *dev, bool enabled);

/**
 * @brief Read active internal ADC resolution
 * @param dev pointer to the device structure
 * @param resolution pointer to variable to keep the resolution, [9-12]
 * @return Exit driver status
 */
nst175_status_t NST175_ResolutionGet(nst175_t *dev, uint8_t *resolution);

/**
 * @brief Write new internal ADC resolution
 * @param dev pointer to the device structure
 * @param resolution new internal ADC resolution, [9-12]
 * @return Exit driver status
 */
nst175_status_t NST175_ResolutionSet(nst175_t *dev, uint8_t resolution);

/**
 * @brief Read active temperature alert limit
 * @param dev pointer to the device structure
 * @param limitType `NST175_LIMIT_LOW` or `NST175_LIMIT_HIGH`
 * @param limit pointer to variable to keep the limit, [°C]
 * @return Exit driver status
 * @note 12-bit resolution forced with 1 bit=0.0625°C
 */
nst175_status_t NST175_LimitGet(nst175_t *dev, nst175_limit_t limitType, float *limit);

/**
 * @brief Write new temperature alert limit
 * @param dev pointer to the device structure
 * @param limitType `NST175_LIMIT_LOW` or `NST175_LIMIT_HIGH`
 * @param limit new limit, [°C]
 * @return Exit driver status
 * @note 12-bit resolution forced with 1 bit=0.0625°C
 */
nst175_status_t NST175_LimitSet(nst175_t *dev, nst175_limit_t limitType, float limit);

/**
 * @brief Read active number of fault conditions that will trigger the temperature alert
 * @param dev pointer to the device structure
 * @param faultQueue pointer to variable to keep the number of fault conditions, [1, 2, 4, 6]
 * @return Exit driver status
 */
nst175_status_t NST175_FaultQueueGet(nst175_t *dev, uint8_t *faultQueue);

/**
 * @brief Write new number of fault conditions that will trigger the temperature alert
 * @param dev pointer to the device structure
 * @param faultQueue new number of fault conditions, [1, 2, 4, 6]
 * @return Exit driver status
 */
nst175_status_t NST175_FaultQueueSet(nst175_t *dev, uint8_t faultQueue);

/**
 * @brief Read active thermostat mode setting
 * @param dev pointer to the device structure
 * @param mode pointer to variable to keep the thermostat mode setting
 * @return Exit driver status
 */
nst175_status_t NST175_ThermostatModeGet(nst175_t *dev, nst175_thermostat_mode_t *mode);

/**
 * @brief Write new thermostat mode setting
 * @param dev pointer to the device structure
 * @param mode `NST175_THERMOSTAT_MODE_COMP` or `NST175_THERMOSTAT_MODE_IT`
 * @return Exit driver status
 * @note In comparator mode alert pin is active when T>=T.high and then until T<T.low.
 * @note In interrupt mode alert pin is active when T>T.high or T<T.low and then until read of temperature register
 */
nst175_status_t NST175_ThermostatModeSet(nst175_t *dev, nst175_thermostat_mode_t mode);

/**
 * @brief Read active alert pin polarity setting
 * @param dev pointer to the device structure
 * @param polarity pointer to variable to keep the alert pin polarity setting
 * @return Exit driver status
 */
nst175_status_t NST175_PolarityGet(nst175_t *dev, nst175_alarm_polarity_t *polarity);

/**
 * @brief Write new alert pin polarity setting
 * @param dev pointer to the device structure
 * @param polarity `NST175_ALARM_POLARITY_LOW` or `NST175_ALARM_POLARITY_HIGH`
 * @return Exit driver status
 */
nst175_status_t NST175_PolaritySet(nst175_t *dev, nst175_alarm_polarity_t polarity);

/**
 * @brief Read result of completed temperature conversion
 * @param dev pointer to the device structure
 * @param temperature pointer to variable to keep the temperature, [°C]
 * @param timeout how long to wait for the conversion result in shutdown mode, [ms]
 * @return Exit driver status
 * @note Continuous mode: there is no sense to call this function more often than once per 40ms.
 * @note Shutdown mode: one-shot measurement will be performed and then ready flag will be polled with `timeout` applied
 */
nst175_status_t NST175_TemperatureGet(nst175_t *dev, float *temperature, uint32_t timeout);
#ifdef __cplusplus
}
#endif