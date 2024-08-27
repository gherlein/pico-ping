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

// Define the SX1262 specific registers and commands
#define SX1262_REG_OP_MODE 0x01
#define SX1262_CMD_SET_STANDBY 0x80
#define SX1262_CMD_SET_TX 0x83
#define SX1262_CMD_SET_RX 0x82
#define SX1262_CMD_WRITE_BUFFER 0x0E
#define SX1262_CMD_READ_BUFFER 0x1E
#define SX1262_CMD_GET_IRQ_STATUS 0x12
#define SX1262_CMD_CLEAR_IRQ_STATUS 0x02

#define SX1262_IRQ_TX_DONE 0x08
#define SX1262_IRQ_RX_DONE 0x40
#define SX1262_IRQ_TIMEOUT 0x20

// Function prototypes
void MCU_Init(void);
void SPI_Init(void);
void GPIO_Init(void);
void SX1262_Init(spiContext_s *spiC);
void SX1262_SendPing(void);
bool SX1262_CheckForPong(void);
void SX1262_ClearIrqStatus(void);
void MCU_Delay(uint16_t ms);
void printRadioCmdStatus(uint8_t code);
static inline void cs_select(void);
static inline void cs_deselect(void);

int main(void)
{
    spiContext_s spiC;

    // Initialize the microcontroller
    MCU_Init();

    SPI_Init();
    GPIO_Init();

    // Initialize the SX1262
    SX1262_Init(&spiC);

    while (1)
    {
        // Send a ping
        SX1262_SendPing();

        // Wait for some time (depends on your application)
        MCU_Delay(1000);

        // Check for a pong response
        if (SX1262_CheckForPong())
        {
            // Pong received, do something
            printf("Pong received!\n");
        }
        else
        {
            // No response or timeout
            printf("No response.\n");
        }

        // Wait before sending the next ping
        MCU_Delay(5000);
    }
}

void MCU_Init(void)
{
}

void MCU_Delay(uint16_t ms)
{
    sleep_ms(ms);
}

void SPI_Init(void)
{

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

void SX1262_Init(spiContext_s *spiC)
{
    stdio_init_all();

    printf("RADIO_RESET: %d\n", RADIO_RESET);
    printf("RADIO_MOSI : %d\n", RADIO_MOSI);
    printf("RADIO_MISO : %d\n", RADIO_MISO);
    printf("RADIO_SCLK:  %d\n", RADIO_SCLK);
    printf("RADIO_NSS :  %d\n", RADIO_NSS);
    printf("RADIO_BUSY : %d\n", RADIO_BUSY);
    printf("RADIO_DIO_1: %d\n", RADIO_DIO_1);
    fflush(stdout);
    puts("");

    // Set the SX1262 to standby mode
    // uint8_t buffer[2] = {SX1262_CMD_SET_STANDBY, 0x00};
    // SPI_Transfer(buffer, sizeof(buffer));

    sx126x_hal_reset(&spiC);

    sx126x_chip_status_t status;
    uint8_t s;

    printf("sx126x_set_standby\n");
    s = sx126x_set_standby(&spiC, SX126X_STANDBY_CFG_RC);
    printRadioCmdStatus(s);

    printf("sx126x_get_status\n");
    s = sx126x_get_status(&spiC, &status);
    printRadioCmdStatus(s);

    printf("\n");
    printf("sx126x_set_pkt_type\n");
    printf("setting packet type %x\n", SX126X_PKT_TYPE_LORA);
    s = sx126x_set_pkt_type(&spiC, SX126X_PKT_TYPE_LORA);
    printRadioCmdStatus(s);
    sx126x_pkt_type_t pType;
    sleep_ms(5);

    printf("sx126x_get_pkt_type\n");
    s = sx126x_get_pkt_type(&spiC, &pType);
    printRadioCmdStatus(s);
    printf("packet type: %d\n", pType);

    s = sx126x_set_rf_freq(&spiC, 914200000);
    printRadioCmdStatus(s);

    sx126x_pa_cfg_params_t pa_cfg;
    pa_cfg.device_sel = 0;       // sx1262
    pa_cfg.hp_max = 0x07;        // max power
    pa_cfg.pa_duty_cycle = 0x04; // per datasheet
    pa_cfg.pa_lut = 0x01;
    s = sx126x_set_pa_cfg(&spiC, &pa_cfg);
    printRadioCmdStatus(s);

    s = sx126x_set_tx_params(&spiC, 22, SX126X_RAMP_200_US);
    printRadioCmdStatus(s);

    s = sx126x_set_buffer_base_address(&spiC, 0, 0);
    printRadioCmdStatus(s);

    //    s = sx126x_set_tx_infinite_preamble(&spiC);
    //    printRadioCmdStatus(s);

    char *str = "PING";
    s = sx126x_write_buffer(&spiC, 0, str, 4);
    printRadioCmdStatus(s);

    s = sx126x_set_dio2_as_rf_sw_ctrl(&spiC, true);
    printRadioCmdStatus(s);

    sx126x_mod_params_lora_t params;
    params.bw = SX126X_LORA_BW_250;
    params.sf = SX126X_LORA_SF7;
    params.cr = SX126X_LORA_CR_4_6;
    params.ldro = 0;

    s = sx126x_set_lora_mod_params(&spiC, &params);
    printRadioCmdStatus(s);

    sx126x_pkt_params_lora_t paramsL;
    paramsL.header_type = SX126X_LORA_PKT_EXPLICIT;
    paramsL.crc_is_on = SX126X_GFSK_CRC_OFF;
    paramsL.invert_iq_is_on = 0;
    paramsL.pld_len_in_bytes = 255;
    paramsL.preamble_len_in_symb = 0xFFFF;
    s = sx126x_set_lora_pkt_params(&spiC, &paramsL);

    s = sx126x_set_tx(&spiC, 30000);
    printRadioCmdStatus(s);
}

void GPIO_Init()
{
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

void SX1262_SendPing(void)
{
#ifdef CODE
    uint8_t pingMessage[] = "PING";

    // Write the ping message to the SX1262's buffer
    uint8_t cmdBuffer[2 + sizeof(pingMessage)] = {SX1262_CMD_WRITE_BUFFER, 0x00};
    memcpy(&cmdBuffer[2], pingMessage, sizeof(pingMessage));
    SPI_Transfer(cmdBuffer, sizeof(cmdBuffer));

    // Set the SX1262 to TX mode
    uint8_t txCmd[2] = {SX1262_CMD_SET_TX, 0x00};
    SPI_Transfer(txCmd, sizeof(txCmd));
#endif
}

bool SX1262_CheckForPong(void)
{
#ifdef CODE
    uint8_t irqStatus = 0;

    // Read the IRQ status register
    uint8_t cmdBuffer[2] = {SX1262_CMD_GET_IRQ_STATUS, 0x00};
    SPI_Transfer(cmdBuffer, sizeof(cmdBuffer));
    irqStatus = cmdBuffer[1];

    // Clear the IRQ status
    SX1262_ClearIrqStatus();

    // Check if a RX done IRQ was triggered
    if (irqStatus & SX1262_IRQ_RX_DONE)
    {
        // Read the received message
        uint8_t rxBuffer[64];
        uint8_t readCmd[2] = {SX1262_CMD_READ_BUFFER, 0x00};
        SPI_Transfer(readCmd, sizeof(readCmd));
        SPI_Transfer(rxBuffer, sizeof(rxBuffer));

        // Check if the received message is "PONG"
        if (strncmp((char *)rxBuffer, "PONG", 4) == 0)
        {
            return true;
        }
    }
#endif
    return false;
}

void SX1262_ClearIrqStatus(void)
{
#ifdef CODE
    uint8_t cmdBuffer[2] = {SX1262_CMD_CLEAR_IRQ_STATUS, 0xFF};
    SPI_Transfer(cmdBuffer, sizeof(cmdBuffer));
#endif
}

void printRadioCmdStatus(uint8_t code)
{
    switch (code)
    {
    case SX126X_STATUS_OK:
        printf("SX126X_STATUS_OK\n");
        break;
    case SX126X_STATUS_UNSUPPORTED_FEATURE:
        printf("SX126X_STATUS_UNSUPPORTED_FEATURE\n");
        break;
    case SX126X_STATUS_UNKNOWN_VALUE:
        printf("SX126X_STATUS_UNKNOWN_VALUE\n");
        break;
    case SX126X_STATUS_ERROR:
        printf("SX126X_STATUS_ERROR\n");
        break;
    default:
        printf("SX126X_STATUS_CODE not known (default handler)\n");
        break;
    }
}
