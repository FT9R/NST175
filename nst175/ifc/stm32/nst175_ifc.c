#include "nst175_ifc.h"
#include "cmsis_os.h"
#include "i2c.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

__root __IO GPIO_TypeDef *gpiob = GPIOB;

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

static void NST175_Print(const char *const fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void NST175_SetUp(nst175_t *dev)
{
    osDelay(5000);
    memset(dev, 0, sizeof(*dev));
    dev->interface.handle = &hi2c1;
    dev->interface.read = NST175_Read;
    dev->interface.write = NST175_Write;
    dev->interface.delay = NST175_Delay;
    dev->interface.print = NST175_Print;
    if (NST175_Init(dev, 0b1001111, true) != NST175_STAT_OK)
        Error_Handler();

    NST175_ResolutionSet(dev, 12);
    NST175_LimitSet(dev, NST175_LIMIT_LOW, 50.0f);
    NST175_LimitSet(dev, NST175_LIMIT_HIGH, 63.1875f);
    NST175_FaultQueueSet(dev, 6);
}