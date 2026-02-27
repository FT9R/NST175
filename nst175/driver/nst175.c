#include "nst175.h"
#include <stddef.h>
#include <string.h>

/* Constants */
#define DEVICE_ADDRESS    (uint8_t) 0b0110111
#define DEVICE_ID         (uint8_t) 0x0A
#define I2C_READ_TIMEOUT  1000
#define I2C_WRITE_TIMEOUT 1000
#define ONE_SHOT_TIMEOUT  1000

/* Macro */
#define __FILENAME__                                       \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 \
                             : (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__))
#define READ_REG(REG, DATA, SIZE, ...)                                                                   \
    do                                                                                                   \
    {                                                                                                    \
        if (!dev->interface.read(dev->interface.handle, dev->address, (REG), (uint8_t *) (DATA), (SIZE), \
                                 I2C_READ_TIMEOUT))                                                      \
            ERROR_SET(NST175_ERR_I2C_READ, __VA_ARGS__);                                                 \
    }                                                                                                    \
    while (0)

#define WRITE_REG(REG, DATA, SIZE, ...)                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        if ((SIZE) == 0 || (SIZE) > 4)                                                                       \
            ERROR_SET(NST175_ERR_INTERNAL, __VA_ARGS__);                                                     \
        uint8_t writeBuf[4] = {0};                                                                           \
        uint32_t data = (uint32_t) (DATA);                                                                   \
        writeBuf[0] = (uint8_t) (data >> 24);                                                                \
        writeBuf[1] = (uint8_t) (data >> 16);                                                                \
        writeBuf[2] = (uint8_t) (data >> 8);                                                                 \
        writeBuf[3] = (uint8_t) (data);                                                                      \
        if (!dev->interface.write(dev->interface.handle, dev->address, (REG), &writeBuf[4 - (SIZE)], (SIZE), \
                                  I2C_WRITE_TIMEOUT))                                                        \
            ERROR_SET(NST175_ERR_I2C_WRITE, __VA_ARGS__);                                                    \
    }                                                                                                        \
    while (0)

#define ERROR_SET(ERR, ...)                                              \
    do                                                                   \
    {                                                                    \
        dev->error |= (ERR);                                             \
        if (dev->callbacks.error != NULL)                                \
            dev->callbacks.error(dev, __FILENAME__, __func__, __LINE__); \
        return __VA_ARGS__;                                              \
    }                                                                    \
    while (0)

/* Custom types */
typedef enum {
    REG_TEMPERATURE = 0b0,
    REG_CONFIGURATION = 0b1,
    REG_TLOW = 0b10,
    REG_THIGH = 0b11,
    REG_ID = 0b111
} regAddr_t;

typedef enum {
    CONFIG_MASK_ONE_SHOT = 0b10000000,
    CONFIG_MASK_RESOLUTION = 0b01100000,
    CONFIG_MASK_FAULT_QUEUE = 0b00011000,
    CONFIG_MASK_ALERT_POL = 0b00000100,
    CONFIG_MASK_MODE = 0b00000010,
    CONFIG_MASK_SHUTDOWN = 0b00000001
} configMask_t;

void NST175_Init(nst175_t *dev)
{
    uint8_t id;
    uint8_t responseAttempt = 0;

    if (dev == NULL)
        return;

    /* Check platform */
    if (dev->interface.read == NULL || dev->interface.write == NULL || dev->interface.delay == NULL)
        ERROR_SET(NST175_ERR_PLATFORM);

    /* Force default values if not provided */
    dev->address = (dev->address == 0) ? DEVICE_ADDRESS : dev->address;
    dev->oneshotTimeout = (dev->oneshotTimeout == 0) ? ONE_SHOT_TIMEOUT : dev->oneshotTimeout;

    /* Clear cached values and errors */
    memset(&dev->resolution, 0, sizeof(*dev) - offsetof(nst175_t, resolution));

    /* Device ID check */
    while (true)
    {
        dev->interface.delay(100);
        READ_REG(REG_ID, &id, 1);

        // uint8_t revision = id & 0x0F;
        id = (id >> 4) & 0x0F;
        if (id == DEVICE_ID)
            break;

        if (++responseAttempt >= 5)
            ERROR_SET(NST175_ERR_ID);
    }

    NST175_ShutdownModeGet(dev);
    NST175_ResolutionGet(dev);
}

void NST175_ShutdownModeGet(nst175_t *dev)
{
    uint8_t config;

    if (dev == NULL)
        return;

    /* Update shutdown mode in device handle */
    READ_REG(REG_CONFIGURATION, &config, 1);
    dev->shutdown = (config & CONFIG_MASK_SHUTDOWN) ? true : false;
}

void NST175_ShutdownModeSet(nst175_t *dev, bool enabled)
{
    uint8_t config;

    if (dev == NULL)
        return;

    /* Write new shutdown mode to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_SHUTDOWN) | (enabled << 0);
    WRITE_REG(REG_CONFIGURATION, config, 1);

    /* Update shutdown mode in device handle */
    NST175_ShutdownModeGet(dev);
}

void NST175_ResolutionGet(nst175_t *dev)
{
    uint8_t config;

    if (dev == NULL)
        return;

    /* Update resolution in device handle */
    READ_REG(REG_CONFIGURATION, &config, 1);
    dev->resolution = ((config & CONFIG_MASK_RESOLUTION) >> 5) + 9;

    /* Update LSB weight in device handle */
    dev->lsb = 1.0f / (1 << (dev->resolution - 8));
}

void NST175_ResolutionSet(nst175_t *dev, uint8_t resolution)
{
    uint8_t config;

    if (dev == NULL)
        return;
    if (resolution < 9 || resolution > 12)
        ERROR_SET(NST175_ERR_ARG);

    /* Write new resolution to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_RESOLUTION) | ((resolution - 9) << 5);
    WRITE_REG(REG_CONFIGURATION, config, 1);

    /* Update resolution in device handle */
    NST175_ResolutionGet(dev);
}

void NST175_LimitGet(nst175_t *dev, nst175_limit_t limitType, float *limit)
{
    uint8_t buf[2];
    int16_t raw;
    regAddr_t reg;

    if (dev == NULL)
        return;
    if (limit == NULL)
        ERROR_SET(NST175_ERR_ARG);

    /* Select register based on limit type */
    switch (limitType)
    {
    case NST175_LIMIT_LOW:
        reg = REG_TLOW;
        break;
    case NST175_LIMIT_HIGH:
        reg = REG_THIGH;
        break;
    default:
        ERROR_SET(NST175_ERR_ARG);
    }

    /* Read 12-bit temperature limit register */
    READ_REG(reg, buf, sizeof(buf));

    /* Assemble MSB-first */
    raw = (int16_t) ((buf[0] << 8) | buf[1]);

    /* Shift right to remove unused bits */
    raw >>= (16 - 12);

    /* Convert raw value to temperature limit */
    *limit = raw * 0.0625f;
}

void NST175_LimitSet(nst175_t *dev, nst175_limit_t limitType, float limit)
{
    int16_t raw;
    regAddr_t reg;

    if (dev == NULL)
        return;

    /* Select register based on limit type */
    switch (limitType)
    {
    case NST175_LIMIT_LOW:
        reg = REG_TLOW;
        break;
    case NST175_LIMIT_HIGH:
        reg = REG_THIGH;
        break;
    default:
        ERROR_SET(NST175_ERR_ARG);
    }

    /* Convert temperature limit to raw value */
    raw = (int16_t) (limit / 0.0625f);

    /* Align to 16-bit left-justified register format */
    raw <<= (16 - 12);

    /* Write new limit to device */
    WRITE_REG(reg, raw, 2);
}

void NST175_FaultQueueGet(nst175_t *dev, uint8_t *faultQueue)
{
    uint8_t config;
    static const uint8_t faultQueueLut[] = {1, 2, 4, 6};

    if (dev == NULL)
        return;
    if (faultQueue == NULL)
        ERROR_SET(NST175_ERR_ARG);

    /* Read fault queue setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *faultQueue = faultQueueLut[((config & CONFIG_MASK_FAULT_QUEUE) >> 3) & 0b11];
}

void NST175_FaultQueueSet(nst175_t *dev, uint8_t faultQueue)
{
    uint8_t config;
    uint8_t encoded;

    if (dev == NULL)
        return;

    switch (faultQueue)
    {
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
        ERROR_SET(NST175_ERR_ARG);
    }

    /* Write new fault queue setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_FAULT_QUEUE) | (encoded << 3);
    WRITE_REG(REG_CONFIGURATION, config, 1);
}

void NST175_ThermostatModeGet(nst175_t *dev, nst175_thermostat_mode_t *mode)
{
    uint8_t config;

    if (dev == NULL)
        return;
    if (mode == NULL)
        ERROR_SET(NST175_ERR_ARG);

    /* Read thermostat mode setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *mode = (config & CONFIG_MASK_MODE) ? NST175_THERMOSTAT_MODE_IT : NST175_THERMOSTAT_MODE_COMP;
}

void NST175_ThermostatModeSet(nst175_t *dev, nst175_thermostat_mode_t mode)
{
    uint8_t config;

    if (dev == NULL)
        return;

    /* Write new thermostat mode setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_MODE) | ((mode == NST175_THERMOSTAT_MODE_IT) << 1);
    WRITE_REG(REG_CONFIGURATION, config, 1);
}

void NST175_PolarityGet(nst175_t *dev, nst175_alarm_polarity_t *polarity)
{
    uint8_t config;

    if (dev == NULL)
        return;
    if (polarity == NULL)
        ERROR_SET(NST175_ERR_ARG);

    /* Read alarm pin polarity setting from device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    *polarity = (config & CONFIG_MASK_ALERT_POL) ? NST175_ALARM_POLARITY_HIGH : NST175_ALARM_POLARITY_LOW;
}

void NST175_PolaritySet(nst175_t *dev, nst175_alarm_polarity_t polarity)
{
    uint8_t config;

    if (dev == NULL)
        return;

    /* Write new alarm pin polarity setting to device */
    READ_REG(REG_CONFIGURATION, &config, 1);
    config = (config & ~CONFIG_MASK_ALERT_POL) | ((polarity == NST175_ALARM_POLARITY_HIGH) << 2);
    WRITE_REG(REG_CONFIGURATION, config, 1);
}

void NST175_TemperatureGet(nst175_t *dev, float *temperature)
{
    uint32_t timer = 0;
    uint8_t config;
    uint8_t buf[2];
    int16_t raw;

    if (dev == NULL)
        return;
    if (temperature == NULL)
        ERROR_SET(NST175_ERR_ARG);

    /* Generate one-shot measurement request if device is in shutdown mode */
    if (dev->shutdown)
    {
        /* Start one-shot measurement */
        READ_REG(REG_CONFIGURATION, &config, 1);
        config |= CONFIG_MASK_ONE_SHOT;
        WRITE_REG(REG_CONFIGURATION, config, 1);

        /* Wait for sample ready */
        while (true)
        {
            dev->interface.delay(1), ++timer;
            READ_REG(REG_CONFIGURATION, &config, 1);
            /* Do not trust datasheet here !!!
             * It states: "During the conversion, the OS bit reads 0. At the end of the conversion, the OS bit reads 1."
             * But in reality its opposite - OS bit is 1 during conversion and goes to 0 when data is ready. */
            if (!(config & CONFIG_MASK_ONE_SHOT))
                break;
            if (timer >= dev->oneshotTimeout)
                ERROR_SET(NST175_ERR_ONESHOT_TIMEOUT);
        }
    }

    /* Read 12-bit temperature register */
    READ_REG(REG_TEMPERATURE, buf, sizeof(buf));

    /* Assemble MSB-first */
    raw = (int16_t) ((buf[0] << 8) | buf[1]);

    /* Shift right to remove unused bits */
    raw >>= (16 - dev->resolution);

    /* Convert raw value to temperature */
    *temperature = raw * dev->lsb;
}
