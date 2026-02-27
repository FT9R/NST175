#pragma once

#include <stdbool.h>
#include <stdint.h>

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
        bool (*read)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        bool (*write)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
        void (*delay)(uint32_t ms);
        void *handle; // Optional user-defined handle for I2C interface
    } interface;

    struct {
        void (*error)(struct nst175_s *dev, const char *file, const char *func, int line);
    } callbacks;

    uint8_t address; // Optional device address. Provide before initialization if address pins are used (not floating)
    uint32_t oneshotTimeout; // Optional timeout for one-shot measurement [ms]
    uint8_t resolution; // Cached resolution value (9-12)
    float lsb; // Cached LSB weight for temperature calculation
    bool shutdown; // Cached shutdown mode state
    nst175_error_t error; // Accumulated driver errors
} nst175_t;

void NST175_Init(nst175_t *dev);

void NST175_ShutdownModeGet(nst175_t *dev);
void NST175_ShutdownModeSet(nst175_t *dev, bool enabled);

void NST175_ResolutionGet(nst175_t *dev);
void NST175_ResolutionSet(nst175_t *dev, uint8_t resolution);

void NST175_LimitGet(nst175_t *dev, nst175_limit_t limitType, float *limit);
void NST175_LimitSet(nst175_t *dev, nst175_limit_t limitType, float limit);

void NST175_FaultQueueGet(nst175_t *dev, uint8_t *faultQueue);
void NST175_FaultQueueSet(nst175_t *dev, uint8_t faultQueue);

void NST175_ThermostatModeGet(nst175_t *dev, nst175_thermostat_mode_t *mode);
void NST175_ThermostatModeSet(nst175_t *dev, nst175_thermostat_mode_t mode);

void NST175_PolarityGet(nst175_t *dev, nst175_alarm_polarity_t *polarity);
void NST175_PolaritySet(nst175_t *dev, nst175_alarm_polarity_t polarity);

void NST175_TemperatureGet(nst175_t *dev, float *temperature);
