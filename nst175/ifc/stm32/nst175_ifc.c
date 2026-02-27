#include "nst175_ifc.h"
#include "cmsis_os.h"
#include "i2c.h"
#include <stdio.h>
#include <string.h>

static bool NST175_Read(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (handle == NULL || data == NULL || size == 0)
        return false;

    return HAL_I2C_Mem_Read(handle, address << 1, reg, I2C_MEMADD_SIZE_8BIT, data, size, timeout) == HAL_OK;
}

static bool NST175_Write(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (handle == NULL || data == NULL || size == 0)
        return false;

    return HAL_I2C_Mem_Write(handle, address << 1, reg, I2C_MEMADD_SIZE_8BIT, data, size, timeout) == HAL_OK;
}

static void NST175_Delay(uint32_t ms)
{
    osDelay(ms);
}

static void NST175_ErrorHandler(nst175_t *dev, const char *file, const char *func, int line)
{
#ifndef NDEBUG
    printf("Error 0x%08X at %s:%s():%d\n", (dev->error), file, func, line);
#endif
    osDelay(1000);
    HAL_I2C_DeInit(&hi2c1);
    MX_I2C1_Init();
    dev->error = NST175_ERR_NONE;
}

__root __IO GPIO_TypeDef *gpiob = GPIOB;

void NST175_SetUp(nst175_t *dev)
{
    osDelay(5000);
    memset(dev, 0, sizeof(*dev));
    dev->interface.handle = &hi2c1;
    dev->interface.read = NST175_Read;
    dev->interface.write = NST175_Write;
    dev->interface.delay = NST175_Delay;
    dev->callbacks.error = NST175_ErrorHandler;
    dev->address = 0b1001111;
    NST175_Init(dev);
    NST175_FaultQueueSet(dev, 6);
    NST175_ResolutionSet(dev, 12);
    NST175_ShutdownModeSet(dev, true);

    float limit = 0;
    NST175_LimitSet(dev, NST175_LIMIT_LOW, 20.625f);
    NST175_LimitSet(dev, NST175_LIMIT_HIGH, 63.1875f);
    NST175_LimitGet(dev, NST175_LIMIT_LOW, &limit);
    NST175_LimitGet(dev, NST175_LIMIT_HIGH, &limit);

    static float temp = 0;
    while (true)
    {
        // osDelay(123);
        NST175_TemperatureGet(dev, &temp);
    }
}