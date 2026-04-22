#include "nst175.h"
#include <stddef.h>

/* Constants */
#define HANDLE_IDENTITY   (uint32_t) 0x17A65F03
#define I2C_READ_TIMEOUT  (uint32_t) 1000
#define I2C_WRITE_TIMEOUT (uint32_t) 1000
#define DEVICE_ID         (uint8_t) 0x0A

/* Macro */
#define HANDLE_CHECK                                               \
    do {                                                           \
        if (dev == NULL || dev->cache.identity != HANDLE_IDENTITY) \
            ERROR_SET(NST175_STAT_HANDLE_NF);                      \
    }                                                              \
    while (0)

#define READ_REG(REG, DATA, SIZE, ...)                                                                           \
    do {                                                                                                         \
        if (!dev->interface.read(dev->interface.handle, dev->cache.address, (uint8_t) (REG), (uint8_t *) (DATA), \
                                 (uint8_t) (SIZE), I2C_READ_TIMEOUT))                                            \
            ERROR_SET(NST175_STAT_I2C_FAIL);                                                                     \
    }                                                                                                            \
    while (0)

#define WRITE_REG(REG, DATA, SIZE, ...)                                                                               \
    do {                                                                                                              \
        uint8_t _writeBuf[4];                                                                                         \
        uint8_t _size = (uint8_t) (SIZE);                                                                             \
        uint32_t _data = (uint32_t) (DATA);                                                                           \
        _writeBuf[0] = (uint8_t) (_data >> 24);                                                                       \
        _writeBuf[1] = (uint8_t) (_data >> 16);                                                                       \
        _writeBuf[2] = (uint8_t) (_data >> 8);                                                                        \
        _writeBuf[3] = (uint8_t) (_data);                                                                             \
        if (!dev->interface.write(dev->interface.handle, dev->cache.address, (uint8_t) (REG), &_writeBuf[4u - _size], \
                                  _size, I2C_WRITE_TIMEOUT))                                                          \
            ERROR_SET(NST175_STAT_I2C_FAIL);                                                                          \
    }                                                                                                                 \
    while (0)

#define PRINT(...)                             \
    do {                                       \
        if (dev->interface.print)              \
            dev->interface.print(__VA_ARGS__); \
    }                                          \
    while (0)

#define ERROR_SET(ERR)                                        \
    do {                                                      \
        nst175_status_t _err = (ERR);                         \
        PRINT("Error %u occurred in %s()\n", _err, __func__); \
        return _err;                                          \
    }                                                         \
    while (0)

/* Custom types */
typedef enum regAddr_e {
    REG_TEMPERATURE = 0b0,
    REG_CONFIGURATION = 0b1,
    REG_TLOW = 0b10,
    REG_THIGH = 0b11,
    REG_ID = 0b111
} regAddr_t;

typedef enum configMask_e {
    CONFIG_MASK_ONE_SHOT = 0b10000000,
    CONFIG_MASK_RESOLUTION = 0b01100000,
    CONFIG_MASK_FAULT_QUEUE = 0b00011000,
    CONFIG_MASK_ALERT_POL = 0b00000100,
    CONFIG_MASK_MODE = 0b00000010,
    CONFIG_MASK_SHUTDOWN = 0b00000001
} configMask_t;

nst175_status_t NST175_Init(nst175_t *dev, uint8_t address)
{
    uint8_t id;
    uint8_t config;
    uint8_t responseAttempt = 0;

    if (dev == NULL)
        return NST175_STAT_HANDLE_NF;

    /* Check basic platform */
    if (dev->interface.read == NULL || dev->interface.write == NULL || dev->interface.delay == NULL)
        ERROR_SET(NST175_STAT_PLATFORM_NF);

    /* Device ID check */
    dev->cache.address = address;
    while (true) {
        dev->interface.delay(100);
        READ_REG(REG_ID, &id, 1);
        id = (id >> 4) & 0x0F;
        if (id == DEVICE_ID)
            break;

        if (++responseAttempt >= 5)
            ERROR_SET(NST175_STAT_ID_NS);
    }

    /* Update cache */
    READ_REG(REG_CONFIGURATION, &config, 1);
    dev->cache.resolution = ((config & CONFIG_MASK_RESOLUTION) >> 5) + 9;
    dev->cache.lsb = 1.0f / (1 << (dev->cache.resolution - 8));
    dev->cache.shutdown = (config & CONFIG_MASK_SHUTDOWN) ? true : false;
    dev->cache.identity = HANDLE_IDENTITY;

    PRINT("NST175 initial resolution: %u\n", dev->cache.resolution);
    PRINT("NST175 initial shutdown mode: %u\n", dev->cache.shutdown);
    return NST175_STAT_OK;
}

nst175_status_t NST175_ShutdownModeGet(nst175_t *dev, bool *enabled)
{
    HANDLE_CHECK;
    if (enabled == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    *enabled = dev->cache.shutdown;
    return NST175_STAT_OK;
}

nst175_status_t NST175_ShutdownModeSet(nst175_t *dev, bool enabled)
{
    uint8_t config;
    HANDLE_CHECK;

    /* Write new shutdown mode to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_SHUTDOWN) | (enabled << 0);
    WRITE_REG(REG_CONFIGURATION, config, 1);

    /* Update shutdown mode in device handle */
    READ_REG(REG_CONFIGURATION, &config, 1);
    dev->cache.shutdown = (config & CONFIG_MASK_SHUTDOWN) ? true : false;
    return NST175_STAT_OK;
}

nst175_status_t NST175_ResolutionGet(nst175_t *dev, uint8_t *resolution)
{
    HANDLE_CHECK;
    if (resolution == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    *resolution = dev->cache.resolution;
    return NST175_STAT_OK;
}

nst175_status_t NST175_ResolutionSet(nst175_t *dev, uint8_t resolution)
{
    uint8_t config;
    HANDLE_CHECK;
    if (resolution < 9 || resolution > 12)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Write new resolution to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (uint8_t) (config & ~CONFIG_MASK_RESOLUTION) | ((resolution - 9) << 5);
    WRITE_REG(REG_CONFIGURATION, config, 1);

    /* Update resolution and lsb weight in device handle */
    READ_REG(REG_CONFIGURATION, &config, 1);
    dev->cache.resolution = ((config & CONFIG_MASK_RESOLUTION) >> 5) + 9;
    dev->cache.lsb = 1.0f / (1 << (dev->cache.resolution - 8));
    return NST175_STAT_OK;
}

nst175_status_t NST175_LimitGet(nst175_t *dev, nst175_limit_t limitType, float *limit)
{
    uint8_t buf[2];
    int16_t raw;
    regAddr_t reg;
    HANDLE_CHECK;
    if (limit == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Select register based on limit type */
    switch (limitType) {
    case NST175_LIMIT_LOW:
        reg = REG_TLOW;
        break;
    case NST175_LIMIT_HIGH:
        reg = REG_THIGH;
        break;
    default:
        ERROR_SET(NST175_STAT_ARG_INVALID);
    }

    /* Read 12-bit temperature limit register */
    READ_REG(reg, buf, sizeof(buf));

    /* Assemble MSB-first */
    raw = (int16_t) (((uint16_t) buf[0] << 8) | buf[1]);

    /* Shift right to remove unused bits */
    raw >>= (16 - 12);

    /* Convert raw value to temperature limit */
    *limit = raw * 0.0625f;
    return NST175_STAT_OK;
}

nst175_status_t NST175_LimitSet(nst175_t *dev, nst175_limit_t limitType, float limit)
{
    int16_t raw;
    regAddr_t reg;
    HANDLE_CHECK;

    /* Select register based on limit type */
    switch (limitType) {
    case NST175_LIMIT_LOW:
        reg = REG_TLOW;
        break;
    case NST175_LIMIT_HIGH:
        reg = REG_THIGH;
        break;
    default:
        ERROR_SET(NST175_STAT_ARG_INVALID);
    }

    /* Convert temperature limit to raw value */
    raw = (int16_t) (limit / 0.0625f);

    /* Align to 16-bit left-justified register format */
    raw <<= (16 - 12);

    /* Write new limit to device */
    WRITE_REG(reg, raw, 2);
    return NST175_STAT_OK;
}

nst175_status_t NST175_FaultQueueGet(nst175_t *dev, uint8_t *faultQueue)
{
    uint8_t config;
    const uint8_t FAULT_QUEUE_LUT[] = {1, 2, 4, 6};
    HANDLE_CHECK;
    if (faultQueue == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Read fault queue setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *faultQueue = FAULT_QUEUE_LUT[((config & CONFIG_MASK_FAULT_QUEUE) >> 3) & 0b11];
    return NST175_STAT_OK;
}

nst175_status_t NST175_FaultQueueSet(nst175_t *dev, uint8_t faultQueue)
{
    uint8_t config;
    uint8_t encoded;
    HANDLE_CHECK;

    switch (faultQueue) {
    case 1:
        encoded = 0b00;
        break;
    case 2:
        encoded = 0b01;
        break;
    case 4:
        encoded = 0b10;
        break;
    case 6:
        encoded = 0b11;
        break;
    default:
        ERROR_SET(NST175_STAT_ARG_INVALID);
    }

    /* Write new fault queue setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (uint8_t) (config & ~CONFIG_MASK_FAULT_QUEUE) | (encoded << 3);
    WRITE_REG(REG_CONFIGURATION, config, 1);
    return NST175_STAT_OK;
}

nst175_status_t NST175_ThermostatModeGet(nst175_t *dev, nst175_thermostat_mode_t *mode)
{
    uint8_t config;
    HANDLE_CHECK;
    if (mode == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Read thermostat mode setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *mode = (config & CONFIG_MASK_MODE) ? NST175_THERMOSTAT_MODE_IT : NST175_THERMOSTAT_MODE_COMP;
    return NST175_STAT_OK;
}

nst175_status_t NST175_ThermostatModeSet(nst175_t *dev, nst175_thermostat_mode_t mode)
{
    uint8_t config;
    HANDLE_CHECK;

    /* Write new thermostat mode setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (uint8_t) (config & ~CONFIG_MASK_MODE) | ((mode == NST175_THERMOSTAT_MODE_IT) << 1);
    WRITE_REG(REG_CONFIGURATION, config, 1);
    return NST175_STAT_OK;
}

nst175_status_t NST175_PolarityGet(nst175_t *dev, nst175_alarm_polarity_t *polarity)
{
    uint8_t config;
    HANDLE_CHECK;
    if (polarity == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Read alarm pin polarity setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *polarity = (config & CONFIG_MASK_ALERT_POL) ? NST175_ALARM_POLARITY_HIGH : NST175_ALARM_POLARITY_LOW;
    return NST175_STAT_OK;
}

nst175_status_t NST175_PolaritySet(nst175_t *dev, nst175_alarm_polarity_t polarity)
{
    uint8_t config;
    HANDLE_CHECK;

    /* Write new alarm pin polarity setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (uint8_t) (config & ~CONFIG_MASK_ALERT_POL) | ((polarity == NST175_ALARM_POLARITY_HIGH) << 2);
    WRITE_REG(REG_CONFIGURATION, config, 1);
    return NST175_STAT_OK;
}

nst175_status_t NST175_TemperatureGet(nst175_t *dev, float *temperature, uint32_t timeout)
{
    uint8_t config;
    uint8_t buf[2];
    int16_t raw;
    const uint8_t pollPeriod = 1;
    HANDLE_CHECK;
    if (temperature == NULL)
        ERROR_SET(NST175_STAT_ARG_INVALID);

    /* Generate one-shot measurement request if device is in shutdown mode */
    if (dev->cache.shutdown) {
        /* Start one-shot measurement */
        READ_REG(REG_CONFIGURATION, &config, 1);
        config |= CONFIG_MASK_ONE_SHOT;
        WRITE_REG(REG_CONFIGURATION, config, 1);

        /* Wait for sample ready */
        while (true) {
            READ_REG(REG_CONFIGURATION, &config, 1);

            /* Do not trust datasheet here !!!
             * It states: "During the conversion, the OS bit reads 0. At the end of the conversion, the OS bit reads 1."
             * But in reality its opposite - OS bit is 1 during conversion and goes to 0 when data is ready. */
            if (!(config & CONFIG_MASK_ONE_SHOT))
                break;

            /* Timeout handling */
            if (timeout < pollPeriod)
                ERROR_SET(NST175_STAT_TIMEOUT);
            dev->interface.delay(pollPeriod);
            timeout -= pollPeriod;
        }
    }

    /* Read 12-bit temperature register */
    READ_REG(REG_TEMPERATURE, buf, sizeof(buf));

    /* Assemble MSB-first */
    raw = (int16_t) (((uint16_t) buf[0] << 8) | buf[1]);

    /* Shift right to remove unused bits */
    raw >>= (16 - dev->cache.resolution);

    /* Convert raw value to temperature */
    *temperature = raw * dev->cache.lsb;
    return NST175_STAT_OK;
}