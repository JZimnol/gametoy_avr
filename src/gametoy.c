#include <avr/io.h>

#include "avrtos/avrtos_delay.h"
#include "avrtos/avrtos_init.h"

#include "buttons.h"
#include "gametoy.h"
#include "spi.h"
#include "utils.h"
#include "welcome_screen.h"

AVRTOS_TASK_DEFINE(display_task);
AVRTOS_STACK_DEFINE(display_thread_stack, AVRTOS_MINIMAL_STACK_SIZE);
AVRTOS_TASK_DEFINE(control_task);
AVRTOS_STACK_DEFINE(control_thread_stack, AVRTOS_MINIMAL_STACK_SIZE + 50);

static gametoy_actions_t *game_actions[_GAME_TYPE_COUNT] = {};
static game_type_t current_game_type = GAME_TYPE_NONE;
static bool game_started = false;

static uint16_t gametoy_framebuffer[GAMETOY_DISPLAY_SIZE];

const static uint8_t DIGITS_BITMAP[10][5] = {
        [0] = {0b11100000, 0b10100000, 0b10100000, 0b10100000, 0b11100000},
        [1] = {0b01000000, 0b11000000, 0b01000000, 0b01000000, 0b11100000},
        [2] = {0b11100000, 0b00100000, 0b11100000, 0b10000000, 0b11100000},
        [3] = {0b11100000, 0b00100000, 0b11100000, 0b00100000, 0b11100000},
        [4] = {0b10100000, 0b10100000, 0b11100000, 0b00100000, 0b00100000},
        [5] = {0b11100000, 0b10000000, 0b11100000, 0b00100000, 0b11100000},
        [6] = {0b11100000, 0b10000000, 0b11100000, 0b10100000, 0b11100000},
        [7] = {0b11100000, 0b10100000, 0b00100000, 0b00100000, 0b00100000},
        [8] = {0b11100000, 0b10100000, 0b11100000, 0b10100000, 0b11100000},
        [9] = {0b11100000, 0b10100000, 0b11100000, 0b00100000, 0b11100000}};

static void display_thread(void *_arg) {
    (void) _arg;

    static uint8_t iterator_spi;

    while (1) {
        AVRTOS_NON_PREEMPTIVE_SECTION() {
            spi_master_tx_16bits_blocking(~gametoy_framebuffer[iterator_spi]);
            spi_master_tx_32bits_blocking(0x80000000 >> iterator_spi);
            spi_latch_trigger();

            iterator_spi++;
            if (iterator_spi == GAMETOY_DISPLAY_SIZE) {
                iterator_spi = 0;
            }
        }

        avrtos_delay_us(300);
    }
}

static void right_button_perform_action(void) {
    if (!game_actions[current_game_type]) {
        return;
    }
    if (game_actions[current_game_type]->right_button_action) {
        game_actions[current_game_type]->right_button_action();
    }
}

static void left_button_perform_action(void) {
    if (!game_actions[current_game_type]) {
        return;
    }
    if (game_actions[current_game_type]->left_button_action) {
        game_actions[current_game_type]->left_button_action();
    }
}

static void up_button_perform_action(void) {
    if (!game_actions[current_game_type]) {
        return;
    }
    if (game_actions[current_game_type]->up_button_action) {
        game_actions[current_game_type]->up_button_action();
    }
}

static void down_button_perform_action(void) {
    if (!game_actions[current_game_type]) {
        return;
    }
    if (game_actions[current_game_type]->down_button_action) {
        game_actions[current_game_type]->down_button_action();
    }
}

static bool periodic_action(uint32_t delay_ms) {
    if (!game_actions[current_game_type]) {
        return false;
    }
    if (game_actions[current_game_type]->right_button_action) {
        return game_actions[current_game_type]->periodic_action(delay_ms);
    }

    return false;
}

static void update_gametoy_framebuffer(void) {
    AVRTOS_NON_PREEMPTIVE_SECTION() {
        if (!game_actions[current_game_type]) {
            return;
        }
        if (game_actions[current_game_type]->update_gametoy_framebuffer) {
            game_actions[current_game_type]->update_gametoy_framebuffer(
                    gametoy_framebuffer);
        }
    }
}

static void initialize_game_one_time() {
    if (!game_actions[current_game_type]) {
        return;
    }
    if (game_actions[current_game_type]->initialize) {
        game_actions[current_game_type]->initialize();
    }
}

static void control_thread(void *_arg) {
    (void) _arg;

    static const uint64_t CONTROL_DELAY_MS = 15;

    welcome_screen_initialize();
    update_gametoy_framebuffer();

    while (1) {
        bool changed = false;
        if (game_started) {
            game_started = false;
            uint16_t seed = (uint16_t) _avrtos_delay_get_microseconds();
            srand(seed);
            initialize_game_one_time();
            update_gametoy_framebuffer();
        }
        if (buttons_right_pushed()) {
            changed = true;
            right_button_perform_action();
        }
        if (buttons_left_pushed()) {
            changed = true;
            left_button_perform_action();
        }
        if (buttons_up_pushed()) {
            changed = true;
            up_button_perform_action();
        }
        if (buttons_down_pushed()) {
            changed = true;
            down_button_perform_action();
        }

        if (periodic_action(CONTROL_DELAY_MS)) {
            changed = true;
        }

        if (changed) {
            changed = false;
            update_gametoy_framebuffer();
        }

        avrtos_delay_ms(CONTROL_DELAY_MS);
    }
}

void gametoy_start(void) {
    welcome_screen_install();
    if (avrtos_task_create(&display_task, display_thread, display_thread_stack,
                           sizeof(display_thread_stack), NULL)) {
        return;
    }

    if (avrtos_task_create(&control_task, control_thread, control_thread_stack,
                           sizeof(control_thread_stack), NULL)) {
        return;
    }

    avrtos_scheduler_start();
}

void gametoy_update_points_framebuffer(uint16_t *framebuffer, uint16_t points) {
    uint8_t unity = points % 10;
    uint8_t tens = (points % 100) / 10;
    uint8_t hundreds = points / 100;

    for (uint8_t i = 0; i < ARRAY_SIZE(DIGITS_BITMAP[0]); i++) {
        framebuffer[i] |= (uint16_t)(DIGITS_BITMAP[hundreds][i]) << 8;
        framebuffer[i] |= (uint16_t)(DIGITS_BITMAP[tens][i]) << 4;
        framebuffer[i] |= (uint16_t)(DIGITS_BITMAP[unity][i]);
    }
}

uint16_t gametoy_get_random_value(void) {
    uint16_t ret;
    AVRTOS_NON_PREEMPTIVE_SECTION() {
        ret = rand();
    }

    return ret;
}

void gametoy_game_over(uint16_t points) {
    for (uint8_t i = 0; i < GAMETOY_DISPLAY_SIZE; i++) {
        AVRTOS_NON_PREEMPTIVE_SECTION() {
            gametoy_framebuffer[i] = 0;
        }
        avrtos_delay_ms(100);
    }
    avrtos_delay_ms(900);

    AVRTOS_NON_PREEMPTIVE_SECTION() {
        gametoy_update_points_framebuffer(&gametoy_framebuffer[1], points);
    }

    while (1) {
        avrtos_delay_ms(10000);
    }
}

void gametoy_game_install(gametoy_actions_t *actions, game_type_t game_type) {
    if (game_type == _GAME_TYPE_COUNT) {
        return;
    }

    game_actions[game_type] = actions;
}

void gametoy_game_select(game_type_t game_type) {
    if (game_type == _GAME_TYPE_COUNT || game_type == GAME_TYPE_NONE) {
        return;
    }

    current_game_type = game_type;
}

void gametoy_game_run() {
    if (current_game_type == _GAME_TYPE_COUNT
        || current_game_type == GAME_TYPE_NONE) {
        return;
    }

    game_started = true;
}
