#include "buttons.h"
#include "gametoy.h"
#include "spi.h"

#include "snake.h"
#include "tetris.h"

int main(void) {
    spi_master_init();
    buttons_init();

    tetris_game_install();
    snake_game_install();

    gametoy_start();

    while (1) {
    }
}
