

#define RADIO_MOSI PICO_DEFAULT_SPI_RX_PIN
#define RADIO_MISO PICO_DEFAULT_SPI_TX_PIN
#define RADIO_SCLK PICO_DEFAULT_SPI_SCK_PIN
#define RADIO_NSS PICO_DEFAULT_SPI_CSN_PIN

#define RADIO_RESET 20
#define RADIO_BUSY 26
#define RADIO_DIO_1 28

typedef struct spiContext_s
{
    int num;
} spiContext_s;

static inline void cs_select(void);
static inline void cs_deselect(void);