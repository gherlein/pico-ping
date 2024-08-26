#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "sx126x_hal.h"
#include "sx126x.h"

#define RADIO_MOSI PICO_DEFAULT_SPI_RX_PIN
#define RADIO_MISO PICO_DEFAULT_SPI_TX_PIN
#define RADIO_SCLK PICO_DEFAULT_SPI_SCK_PIN
#define RADIO_NSS PICO_DEFAULT_SPI_CSN_PIN

#define RADIO_RESET 20
#define RADIO_BUSY 26
#define RADIO_DIO_1 28

void debugPrintf(char *str)
{
    printf("%s\n", str);
    fflush(stdout);
    puts("");
}

#ifdef PICO_DEFAULT_SPI_CSN_PIN
static inline void cs_select()
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0); // Active low
    asm volatile("nop \n nop \n nop");
    // sleep_ms(1);
}

static inline void cs_deselect()
{
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
    // sleep_ms(1);
}

#endif

#if defined(spi_default) && defined(PICO_DEFAULT_SPI_CSN_PIN)
static void write_register(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = reg & 0x7f; // remove read bit as this is a write
    buf[1] = data;
    cs_select();
    spi_write_blocking(spi_default, buf, 2);
    cs_deselect();
    sleep_ms(10);
}

static void read_registers(uint8_t reg, uint8_t *buf, uint16_t len)
{
    cs_select();
    spi_write_blocking(spi_default, &reg, 1);
    sleep_ms(10);
    spi_read_blocking(spi_default, 0, buf, len);
    cs_deselect();
    sleep_ms(10);
}
#endif

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
    debugPrintf("sx126x_hal_reset()");

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

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command, const uint16_t command_length,
                                     const uint8_t *data, const uint16_t data_length)
{

    uint8_t buf[128];
    uint8_t index = 0;

    debugPrintf("sx126x_hal_write()");

    if (command && command_length > 0)
    {
        for (int i = 0; i < command_length; i++)
        {
            buf[index] = command[i];
            index++;
        }
    }
    if (data && command_length > 0)
    {
        for (int i = 1; i < command_length - 1; i++)
        {
            buf[index] = data[i];
        }
    }
    cs_select();
    while (!spi_is_writable(spi_default))
    {
        sleep_us(100);
    }
    spi_write_blocking(spi_default, buf, command_length + data_length);
    cs_deselect();
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command, const uint16_t command_length,
                                    uint8_t *data, const uint16_t data_length)
{
    return SX126X_HAL_STATUS_OK;
}

void initSPI(void)
{
    debugPrintf("initSPI");

    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));

    // Set SPI format
    spi_set_format(spi_default, // SPI instance
                   8,           // Number of bits per transfer
                   0,           // Polarity (CPOL)
                   0,           // Phase (CPHA)
                   SPI_MSB_FIRST);

    return;
}

void initControl(void)
{
    debugPrintf("initControl");

    // RADIO_BUSY is an input
    gpio_init(RADIO_BUSY);
    gpio_set_dir(RADIO_NSS, GPIO_IN);

    // Reset is active-low
    gpio_init(RADIO_RESET);
    gpio_set_dir(RADIO_RESET, GPIO_OUT);
    gpio_put(RADIO_RESET, 1);

    // Chip select NSS is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    // Make the CS pin available to picotool
    // bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));
}

void doIt(void);

int main()
{
    doIt();
}

void doIt(void)
{
    typedef struct spiContext_t
    {
        int num;
    } spiContext;
    spiContext spiC;

    stdio_init_all();
#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning spi example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    printf("RADIO_RESET: %d\n", RADIO_RESET);
    printf("RADIO_MOSI : %d\n", RADIO_MOSI);
    printf("RADIO_MISO : %d\n", RADIO_MISO);
    printf("RADIO_SCLK:  %d\n", RADIO_SCLK);
    printf("RADIO_NSS :  %d\n", RADIO_NSS);
    printf("RADIO_BUSY : %d\n", RADIO_BUSY);
    printf("RADIO_DIO_1: %d\n", RADIO_DIO_1);
    fflush(stdout);
    puts("");

    sx126x_hal_reset(&spiC);
    initSPI();
    initControl();

    sx126x_set_standby(&spiC, SX126X_STANDBY_CFG_RC);
    sx126x_set_pkt_type(&spiC, SX126X_PKT_TYPE_LORA);
    sx126x_set_rf_freq(&spiC, 915000000);
    sx126x_pa_cfg_params_t pa_cfg;
    pa_cfg.device_sel = 0;       // sx1262
    pa_cfg.hp_max = 0x07;        // max power
    pa_cfg.pa_duty_cycle = 0x04; // by code inspection
    pa_cfg.pa_lut = 0x01;
    sx126x_set_pa_cfg(&spiC, &pa_cfg);

    //   sx126x_set_fs(&spiC);

    while (1)
    {
        // printf("CS: %d\n", PICO_DEFAULT_SPI_CSN_PIN);
        // fflush(stdout);
        // puts("");
        // cs_select();
        sleep_ms(20);
        // cs_deselect();
        // sleep_ms(20);
    }

#endif
}

/*
14.2 Circuit Configuration for Basic Tx Operation
This chapter describes the sequence of operations needed to send or receive a frame starting from a power up.
After power up (battery insertion or hard reset) the chip runs automatically a calibration procedure and goes to STDBY_RC
mode. This is indicated by a low state on BUSY pin. From this state the steps are:
1. If not in STDBY_RC mode, then go to this mode with the command SetStandby(...)
2. Define the protocol (LoRa® or FSK) with the command SetPacketType(...)
3. Define the RF frequency with the command SetRfFrequency(...)
4. Define the Power Amplifier configuration with the command SetPaConfig(...)
5. Define output power and ramping time with the command SetTxParams(...)
6. Define where the data payload will be stored with the command SetBufferBaseAddress(...)
7. Send the payload to the data buffer with the command WriteBuffer(...)
8. Define the modulation parameter according to the chosen protocol with the command SetModulationParams(...)1
9. Define the frame format to be used with the command SetPacketParams(...)2
10. Configure DIO and IRQ: use the command SetDioIrqParams(...) to select TxDone IRQ and map this IRQ to a DIO (DIO1,
DIO2 or DIO3)
11. Define Sync Word value: use the command WriteReg(...) to write the value of the register via direct register access
12. Set the circuit in transmitter mode to start transmission with the command SetTx(). Use the parameter to enable
Timeout
13. Wait for the IRQ TxDone or Timeout: once the packet has been sent the chip goes automatically to STDBY_RC mode
14. Clear the IRQ TxDone flag
*/