#include <inttypes.h>
#include <string.h>

#include "gametoy.h"
#include "welcome_screen.h"

typedef enum { DIRECTION_UP, DIRECTION_DOWN } direction_t;

const static uint8_t ARROW_BITMAP[8] = {0b00000000, 0b00001000, 0b00001100,
                                        0b11111110, 0b11111110, 0b00001100,
                                        0b00001000, 0b00000000};

static welcome_screen_get_game_animation_t *games_animations[_GAME_TYPE_COUNT];
static game_type_t installed_games[_GAME_TYPE_COUNT];
static uint8_t installed_games_count;
static uint8_t arrow_index;
static uint16_t welcome_screen_framebuffer[GAMETOY_DISPLAY_SIZE];

static uint8_t *get_animation(game_type_t game) {
    return games_animations[game]();
}

static void move_arrow(direction_t direction) {
    uint8_t previous_arrow_index = arrow_index;
    if (direction == DIRECTION_DOWN) {
        if (arrow_index + 1 == installed_games_count) {
            return;
        }
        arrow_index++;
    } else {
        if (arrow_index + 1 == 1) {
            return;
        }
        arrow_index--;
    }

    for (uint8_t i = 0; i < WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE; i++) {
        welcome_screen_framebuffer[i + 8 * previous_arrow_index] &=
                ~(0xff << 8);
        welcome_screen_framebuffer[i + 8 * arrow_index] |= ARROW_BITMAP[i] << 8;
    }
}

static void welcome_screen_right_button_action(void) {
    gametoy_game_select(installed_games[arrow_index]);
    gametoy_game_run();
}

static void welcome_screen_left_button_action(void) {
    return;
}

static void welcome_screen_up_button_action(void) {
    move_arrow(DIRECTION_UP);
}

static void welcome_screen_down_button_action(void) {
    move_arrow(DIRECTION_DOWN);
}

static void update_animations(void) {
    for (uint8_t i = 0; i < installed_games_count; i++) {
        uint8_t *animation = get_animation(installed_games[i]);
        for (uint8_t j = 0; j < WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE;
             j++) {
            welcome_screen_framebuffer[j + 8 * i] &= 0xff00;
            welcome_screen_framebuffer[j + 8 * i] |= (uint16_t) animation[j];
        }
    }
}

static bool welcome_screen_periodic_action(uint32_t delay_ms) {
    static uint32_t periodic_elapsed_ms;

    periodic_elapsed_ms += delay_ms;
    if (periodic_elapsed_ms < 300) {
        return false;
    }

    periodic_elapsed_ms = 0;
    update_animations();

    return true;
}

static void
welcome_screen_update_gametoy_framebuffer(uint16_t *gametoy_framebuffer) {
    memcpy(gametoy_framebuffer, welcome_screen_framebuffer,
           GAMETOY_DISPLAY_SIZE * sizeof(uint16_t));
}

void welcome_screen_initialize(void) {
    for (game_type_t game = 0; game < _GAME_TYPE_COUNT; game++) {
        installed_games[game] = _GAME_TYPE_COUNT;
        if (games_animations[game]) {
            installed_games[installed_games_count] = game;
            installed_games_count++;
        }
    }

    if (installed_games_count == 0) {
        while (1)
            ;
    }

    for (uint8_t i = 0; i < WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE; i++) {
        welcome_screen_framebuffer[i] = ARROW_BITMAP[i] << 8;
    }

    for (uint8_t i = 0; i < installed_games_count; i++) {
        uint8_t *animation = get_animation(installed_games[i]);
        for (uint8_t j = 0; j < WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE;
             j++) {
            welcome_screen_framebuffer[j + 8 * i] |= (uint16_t) animation[i];
        }
    }
}

void welcome_screen_animation_install(
        welcome_screen_get_game_animation_t *animation, game_type_t game_type) {
    if (game_type == _GAME_TYPE_COUNT || game_type == GAME_TYPE_NONE) {
        return;
    }

    games_animations[game_type] = animation;
}

static gametoy_actions_t welcome_screen_actions = {
        .right_button_action = &welcome_screen_right_button_action,
        .left_button_action = &welcome_screen_left_button_action,
        .up_button_action = &welcome_screen_up_button_action,
        .down_button_action = &welcome_screen_down_button_action,
        .periodic_action = &welcome_screen_periodic_action,
        .update_gametoy_framebuffer =
                &welcome_screen_update_gametoy_framebuffer};

void welcome_screen_install() {
    gametoy_game_install(&welcome_screen_actions, GAME_TYPE_NONE);
}
