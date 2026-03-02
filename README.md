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
- Assign your platform-specific functions:

```C
void *handle; // Optional handle for I2C interface
bool (*read)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
bool (*write)(void *handle, uint16_t address, uint16_t reg, uint8_t *data, uint16_t size, uint32_t timeout);
void (*delay)(uint32_t ms);
```

- Common device setup
```C
void NST175_SetUp(nst175_t *dev)
{
    i2c1_Init(&i2c1Handle, 100000);

    memset(dev, 0, sizeof(*dev));
    dev->interface.handle = &i2c1Handle;
    dev->interface.read = ctx_i2c_read;
    dev->interface.write = ctx_i2c_write;
    dev->interface.delay = ctx_delay;
    dev->address = 0b1001111; // All addr pins are high
    NST175_Init(dev);

    /* Overwrite defaults if needed */
    NST175_ResolutionSet(dev, 12); // Default 9-bit
    NST175_LimitSet(dev, NST175_LIMIT_LOW, 50.0f); // Default 75°C
    NST175_LimitSet(dev, NST175_LIMIT_HIGH, 63.1875f); // Default 80°C
    NST175_FaultQueueSet(dev, 6); // Default 1 fault to trigger alarm
}
```

- Read temperature
```C
void Task_ReadTemperature(void)
{
    float temp;
    nst175_t tempSensor;

    NST175_SetUp(&tempSensor);
    while (1)
    {
        NST175_TemperatureGet(&tempSensor, &temp);
    }
}
```

## Examples
* [`STM32 + FreeRTOS`](/platform/stm32/Core/Src/freertos.c)
