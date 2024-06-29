#ifndef SPI_H_
#define SPI_H_

#include <avr/io.h>

#define SPI_LATCH_ON PORTB |= (1<<PB2)
#define SPI_LATCH_OFF PORTB &= ~(1<<PB2)

void spi_master_init(void);
void spi_master_tx_16bits_blocking(uint16_t data_bytes);
void spi_master_tx_32bits_blocking(uint32_t data_bytes);

void inline spi_latch_trigger(void) {
    SPI_LATCH_ON;
    SPI_LATCH_OFF;
}

#endif /* SPI_H_ */