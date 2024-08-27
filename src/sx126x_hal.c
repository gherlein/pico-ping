#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "sx126x_hal.h"
#include "sx126x.h"
#include "pico-ping.h"

static inline void cs_select(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0); // Active low
    asm volatile("nop \n nop \n nop");
    // sleep_ms(1);
}

static inline void cs_deselect(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
    // sleep_ms(1);
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{

    // 8.1 says you only need to strobe low for 10uS but am over-clubbing this
    sleep_ms(10);
    gpio_put(RADIO_RESET, 0);
    sleep_ms(10);
    gpio_put(RADIO_RESET, 1);
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
    return SX126X_HAL_STATUS_OK;
}

void sx126x_wait_on_busy(void)
{
    // while (gpio_get(RADIO_BUSY, 1))
    // {
    //     // NOP Q: can this be a place we block forever?
    // }
}

void sx126x_check_device_ready(void)
{
#ifdef ORIG
    if ((SX126xGetOperatingMode() == MODE_SLEEP) || (SX126xGetOperatingMode() == MODE_RX_DC))
    {
        SX126xWakeup();
        // Switch is turned off when device is in sleep mode and turned on is all other modes
        SX126xAntSwOn();
    }
    SX126xWaitOnBusy();
#endif
}

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, const uint16_t command_length,
                                     const uint8_t *data, const uint16_t data_length)
{
    printf("sx126x_hal_write()\n");

    int count = 0;
    int max = command_length + data_length;
    uint8_t tx[256];

    printf("c len: %d - d len: %d - max: %d\n", command_length, data_length, max);

    for (int x = 0; x < command_length; x++)
    {
        // printf("[%x] ", command[x]);
        tx[count] = command[x];
        count++;
    }
    printf("\n");
    for (int x = 0; x < data_length; x++)
    {
        // printf("[%x] ", data[x]);
        tx[count] = data[x];
        count++;
    }
    printf("\n");

    cs_select();
    while (!spi_is_writable(spi_default))
    {
        sleep_us(100);
    }

    count = spi_write_blocking(spi_default, tx, count);
    cs_deselect();

    printf("sent: %d\n", count);
    for (int x = 0; x < count; x++)
    {
        printf("[%x] ", tx[x]);
    }
    printf("\n");
    fflush(stdout);
    puts("");
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command, const uint16_t command_length,
                                    uint8_t *data, const uint16_t data_length)
{
    printf("sx126x_hal_read\n");
    int count = 0;

    printf("c len: %d - d len: %d \n", command_length, data_length);

    for (int x = 0; x < command_length; x++)
    {
        printf("[%x] ", command[x]);
    }
    printf("\n");

    cs_select();
    while (!spi_is_writable(spi_default))
    {
        sleep_us(100);
    }

    spi_write_blocking(spi_default, command, command_length);
    count = spi_write_read_blocking(spi_default, command, data, data_length);
    cs_deselect();

    printf("read: ");
    for (int x = 0; x < data_length; x++)
    {
        printf("[%x] ", data[x]);
    }
    printf("\n");
    fflush(stdout);
    puts("");

    return SX126X_HAL_STATUS_OK;
}