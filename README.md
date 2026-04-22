# NST175 Temperature Sensor Driver

Platform-independent, full-featured, dependency-free C driver for the **NOVOSENSE NST175** low-power, high precision digital temperature sensor compatible with SMBus and I2C interfaces.  
Provides initialization, configuration and measurement functions using an abstracted I2C interface.  

## Features

- Platform-independent
- No external library dependencies, except for standard C
- Device ID validation  
- A few devices on the same and different I2C bus support with separated handles
- Timeouts for blocking operations  

## Usage
- Assign platform-specific functions:

```C
void *handle; // Optional handle for I2C interface
bool (*read)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
bool (*write)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
void (*delay)(uint32_t ms);
void (*print)(const char *const fmt, ...); // Optional debug print
```

- Common device setup
```C
void NST175_SetUp(nst175_t *dev)
{
    i2c1_Init(&i2c1Handle, 100000);

    memset(dev, 0, sizeof(*dev));
    dev->interface.handle = &hi2c1;
    dev->interface.read = NST175_Read;
    dev->interface.write = NST175_Write;
    dev->interface.delay = NST175_Delay;
    dev->interface.print = NST175_Print;
    if (NST175_Init(dev, 0b1001111) != NST175_STAT_OK)
        Error_Handler();

    /* Overwrite defaults if needed */
    NST175_ResolutionSet(dev, 12); // Default 9-bit
    NST175_FaultQueueSet(dev, 6); // Default 1 fault to trigger alarm
    NST175_LimitSet(dev, NST175_LIMIT_LOW, 50.0f); // Default 75°C
    NST175_LimitSet(dev, NST175_LIMIT_HIGH, 63.1875f); // Default 80°C
}
```

- Read temperature in Continuous-Conversion Mode (async)
```C
void Task_ReadTemperatureAsync(void)
{
    float temp;
    nst175_t tempSensor;
    TickType_t nextWake;

    NST175_SetUp(&tempSensor);
    nextWake = osKernelGetTickCount();
    while (1) {
        /* Request the temperature */
        NST175_TemperatureGet(&tempSensor, &temp, 0);

        /* Non-driver routines */
        osMessageQueuePut(tempToPidQueueHandle, &temp, 0, 0);
        osMessageQueuePut(tempToCanQueueHandle, &temp, 0, 20);

        /* Delay if needed */
        nextWake += osMsToTicks(NST175_CONVERSION_TIME);
        osDelayUntil(nextWake);
    }
}
```

- Read temperature in Shutdown Mode (polling)
```C
void Task_ReadTemperaturePoll(void)
{
    float temp;
    nst175_t tempSensor;

    NST175_SetUp(&tempSensor);
    NST175_ShutdownModeSet(&tempSensor, true);
    while (1) {
        /* Request the temperature */
        NST175_TemperatureGet(&tempSensor, &temp, NST175_CONVERSION_TIME);

        /* Non-driver routines */
        osMessageQueuePut(tempToPidQueueHandle, &temp, 0, 0);
        osMessageQueuePut(tempToCanQueueHandle, &temp, 0, 20);
    }
}
```

## Examples
* [`STM32 + FreeRTOS`](/platform/stm32/Core/Src/freertos.c)
