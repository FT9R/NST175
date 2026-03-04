#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NST175_CONVERSION_TIME 40

typedef enum {
    NST175_ERR_NONE = 0,
    NST175_ERR_ARG = 1 << 0,
    NST175_ERR_PLATFORM = 1 << 1,
    NST175_ERR_ID = 1 << 2,
    NST175_ERR_I2C_READ = 1 << 3,
    NST175_ERR_I2C_WRITE = 1 << 4,
    NST175_ERR_ONESHOT_TIMEOUT = 1 << 5,
    NST175_ERR_INTERNAL = 1 << 6
} nst175_error_t;

typedef enum { NST175_LIMIT_LOW, NST175_LIMIT_HIGH } nst175_limit_t;

typedef enum { NST175_THERMOSTAT_MODE_COMP, NST175_THERMOSTAT_MODE_IT } nst175_thermostat_mode_t;

typedef enum { NST175_ALARM_POLARITY_LOW, NST175_ALARM_POLARITY_HIGH } nst175_alarm_polarity_t;

typedef struct nst175_s {
    struct {
        void *handle; // Optional handle for I2C interface
        bool (*read)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        bool (*write)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        void (*delay)(uint32_t ms);
    } interface;

    struct {
        void (*error)(struct nst175_s *dev, nst175_error_t error,
                      const char *funcName); // Optional error callback, to be invoked on any driver error occurrence
    } callbacks;

    /* Per-device driver internals - do not modify */
    struct {
        uint8_t resolution; // Resolution value (9-12)
        bool shutdown; // Shutdown mode state
        float lsb; // LSB weight for temperature calculation
        uint32_t identity; // Device handle identity, used by driver to check if `nst175_t` structure can be safely used
    } cache;

    /* Read/Write safe in the same task context */
    uint8_t address; // Optional device address. Provide before initialization if address pins are used (not floating)
    uint32_t oneshotTimeout; // Optional timeout for one-shot measurement [ms], default is 100ms
    nst175_error_t error; // Accumulated driver errors
} nst175_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the NST175 device
 * @param dev pointer to the device structure
 * @note Interface read/write/delay function pointers have to be provided prior `NST175_Init()` call
 */
void NST175_Init(nst175_t *dev);

/**
 * @brief Enable low-power shutdown mode
 * @param dev pointer to the device structure
 * @param enabled `true` to enable
 * @note In shutdown mode using `NST175_TemperatureGet()` will lead to one-shot command to be sent to perform a
temperature transition, and the device shuts down automatically when the temperature transition is complete
 */
void NST175_ShutdownModeSet(nst175_t *dev, bool enabled);

/**
 * @brief Change internal ADC resolution
 * @param dev pointer to the device structure
 * @param resolution `9-12`
 */
void NST175_ResolutionSet(nst175_t *dev, uint8_t resolution);

/**
 * @brief Read active temperature alert limit
 * @param dev pointer to the device structure
 * @param limitType `NST175_LIMIT_LOW` or `NST175_LIMIT_HIGH`
 * @param limit pointer to variable to keep the limit
 * @note 12-bit resolution forced with 1 bit=0.0625°C
 */
void NST175_LimitGet(nst175_t *dev, nst175_limit_t limitType, float *limit);

/**
 * @brief Write new temperature alert limit
 * @param dev pointer to the device structure
 * @param limitType `NST175_LIMIT_LOW` or `NST175_LIMIT_HIGH`
 * @param limit new limit
 * @note 12-bit resolution forced with 1 bit=0.0625°C
 */
void NST175_LimitSet(nst175_t *dev, nst175_limit_t limitType, float limit);

/**
 * @brief Read active number of fault conditions that will trigger the temperature alert
 * @param dev pointer to the device structure
 * @param faultQueue pointer to variable to keep the number of fault conditions
 */
void NST175_FaultQueueGet(nst175_t *dev, uint8_t *faultQueue);

/**
 * @brief Write new number of fault conditions that will trigger the temperature alert
 * @param dev pointer to the device structure
 * @param faultQueue new number of fault conditions
 */
void NST175_FaultQueueSet(nst175_t *dev, uint8_t faultQueue);

/**
 * @brief Read active thermostat mode setting
 * @param dev pointer to the device structure
 * @param mode pointer to variable to keep the thermostat mode setting
 */
void NST175_ThermostatModeGet(nst175_t *dev, nst175_thermostat_mode_t *mode);

/**
 * @brief Write new thermostat mode setting
 * @param dev pointer to the device structure
 * @param mode `NST175_THERMOSTAT_MODE_COMP` or `NST175_THERMOSTAT_MODE_IT`
 * @note In comparator mode alert pin is active when T>=T.high and then until T<T.low.
 * @note In interrupt mode alert pin is active when T>T.high or T<T.low and then until read of temperature register
 */
void NST175_ThermostatModeSet(nst175_t *dev, nst175_thermostat_mode_t mode);

/**
 * @brief Read active alert pin polarity setting
 * @param dev pointer to the device structure
 * @param polarity pointer to variable to keep the alert pin polarity setting
 */
void NST175_PolarityGet(nst175_t *dev, nst175_alarm_polarity_t *polarity);

/**
 * @brief Write new alert pin polarity setting
 * @param dev pointer to the device structure
 * @param polarity `NST175_ALARM_POLARITY_LOW` or `NST175_ALARM_POLARITY_HIGH`
 */
void NST175_PolaritySet(nst175_t *dev, nst175_alarm_polarity_t polarity);

/**
 * @brief Read result of completed temperature conversion
 * @param dev pointer to the device structure
 * @param temperature pointer to variable to keep the temperature
 * @note Usually there is no sense to call this function more often than once per 40ms
 * @note In shutdown mode one-shot measurement will be performed automatically and then ready flag will be polled with
 * `dev.oneshotTimeout`
 */
void NST175_TemperatureGet(nst175_t *dev, float *temperature);

#ifdef __cplusplus
}
#endif