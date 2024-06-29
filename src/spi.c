#include <avr/io.h>

#include "avrtos/avrtos_utils.h"

static inline void spi_master_tx_8bits_blocking(uint8_t data_byte) {
    SPDR = data_byte;
    while(!(SPSR & (1<<SPIF)))
        ;
}

void spi_master_init(void) {
    DDRB |= _BV(PB3);
    DDRB |= _BV(PB5);
    DDRB |= _BV(PB2);

    SPCR |= _BV(DORD);
    SPCR |= _BV(SPR0);
    SPCR |= _BV(MSTR);
    SPCR |= _BV(SPE);
}

void spi_master_tx_16bits_blocking(uint16_t data_bytes) {
    uint8_t *data_bytes_ptr = (uint8_t *)&data_bytes;
    spi_master_tx_8bits_blocking(data_bytes_ptr[1]);
    spi_master_tx_8bits_blocking(data_bytes_ptr[0]);
}

void spi_master_tx_32bits_blocking(uint32_t data_bytes) {
    uint16_t *data_bytes_ptr = (uint16_t *)&data_bytes;
    spi_master_tx_16bits_blocking(data_bytes_ptr[1]);
    spi_master_tx_16bits_blocking(data_bytes_ptr[0]);
}