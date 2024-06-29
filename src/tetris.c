#include <string.h>

#include "gametoy.h"
#include "tetris.h"
#include "utils.h"
#include "welcome_screen.h"

typedef enum {
    BLOCK_TYPE_I,
    BLOCK_TYPE_J,
    BLOCK_TYPE_L,
    BLOCK_TYPE_O,
    BLOCK_TYPE_S,
    BLOCK_TYPE_T,
    BLOCK_TYPE_Z,
    _BLOCK_TYPE_COUNT
} block_type_t;

static const uint8_t BLOCKS_BITMAP[_BLOCK_TYPE_COUNT][2] = {
        [BLOCK_TYPE_I] = {0b11110000, 0b00000000},
        [BLOCK_TYPE_J] = {0b11100000, 0b00100000},
        [BLOCK_TYPE_L] = {0b11100000, 0b10000000},
        [BLOCK_TYPE_O] = {0b01100000, 0b01100000},
        [BLOCK_TYPE_S] = {0b01100000, 0b11000000},
        [BLOCK_TYPE_T] = {0b11100000, 0b01000000},
        [BLOCK_TYPE_Z] = {0b11000000, 0b01100000}};

static const uint16_t WALLS = 0xc003;

static struct {
    uint16_t current_block[24]; // 8-31
    uint16_t old_blocks[24];    // 8-31
    uint16_t next_block[2];     // 3-4
    uint16_t points[5];         // 1-5
    uint8_t animation[WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE];
} framebuffers;

static struct {
    block_type_t block;
    uint8_t x;
    uint8_t y;
} current_block;

static uint16_t points_counter;
static uint32_t periodic_elapsed_ms;
static block_type_t next_block;

static void update_points_framebuffer(void) {
    if (points_counter >= 1000) {
        points_counter = 0;
    }
    memset(framebuffers.points, 0, sizeof(framebuffers.points));
    gametoy_update_points_framebuffer(framebuffers.points, points_counter);
}

static void delete_full_levels(void) {
    uint8_t i = ARRAY_SIZE(framebuffers.old_blocks) - 1;
    int8_t bonus = 0;
    while (i != 0) {
        if ((framebuffers.old_blocks[i] | WALLS) == 0xffff) {
            for (uint8_t j = i; j > 0; j--) {
                framebuffers.old_blocks[j] = framebuffers.old_blocks[j - 1];
            }
            framebuffers.old_blocks[0] = 0x0000;
            points_counter++;
            bonus++;
            continue;
        }
        i--;
    }

    if (--bonus > 0) {
        points_counter += bonus;
    }

    update_points_framebuffer();
}

static void game_over(void) {
    gametoy_game_over(points_counter);
}

static void block_generate_new(void) {
    current_block.block = next_block;
    current_block.x = 7;
    current_block.y = 0;

    memset(framebuffers.current_block, 0, sizeof(framebuffers.current_block));

    for (uint8_t i = 0; i < ARRAY_SIZE(BLOCKS_BITMAP[current_block.block]);
         i++) {
        framebuffers.current_block[i] =
                (uint16_t)(BLOCKS_BITMAP[current_block.block][i]) << 2;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(BLOCKS_BITMAP[current_block.block]);
         i++) {
        if (framebuffers.current_block[i] & framebuffers.old_blocks[i]) {
            game_over();
        }
    }
}

static block_type_t block_generate_random(void) {
    uint16_t random_value = gametoy_get_random_value();
    block_type_t ret = random_value % _BLOCK_TYPE_COUNT;

    return ret;
}

static void block_generate_next(void) {
    next_block = block_generate_random();
    memset(framebuffers.next_block, 0, sizeof(framebuffers.next_block));
    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.next_block); i++) {
        framebuffers.next_block[i] =
                (uint16_t)(BLOCKS_BITMAP[next_block][i]) >> 4;
    }
}

static bool is_space_down(void) {
    if (framebuffers.current_block[ARRAY_SIZE(framebuffers.current_block) - 1]
        != 0) {
        return false;
    }

    for (uint8_t i = 1; i < ARRAY_SIZE(framebuffers.old_blocks); i++) {
        if (framebuffers.old_blocks[i] & framebuffers.current_block[i - 1]) {
            return false;
        }
    }

    return true;
}

static bool is_space_right(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.current_block); i++) {
        if (framebuffers.current_block[i] >> 1 & framebuffers.old_blocks[i]) {
            return false;
        }
        if (framebuffers.current_block[i] >> 1 & WALLS) {
            return false;
        }
    }

    return true;
}

static bool is_space_left(void) {
    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.current_block); i++) {
        if (framebuffers.current_block[i] << 1 & framebuffers.old_blocks[i]) {
            return false;
        }
        if (framebuffers.current_block[i] << 1 & WALLS) {
            return false;
        }
    }

    return true;
}

static bool block_move_down(void) {
    if (is_space_down()) {
        for (uint8_t i = ARRAY_SIZE(framebuffers.current_block) - 1; i > 0;
             i--) {
            framebuffers.current_block[i] = framebuffers.current_block[i - 1];
        }
        framebuffers.current_block[0] = 0x0000;
        current_block.y++;

        return true;
    } else {
        for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.old_blocks); i++) {
            framebuffers.old_blocks[i] |= framebuffers.current_block[i];
        }
    }

    delete_full_levels();

    block_generate_new();
    block_generate_next();

    return false;
}

static void blocks_JSTZL_swap_bits(int8_t x_offset,
                                   int8_t y_offset,
                                   uint16_t *framebuffer_copy) {
    int8_t x_copy_offset = y_offset;
    int8_t y_copy_offset = -x_offset;

    uint8_t bit;
    if (current_block.y == 0 && y_copy_offset == -1) {
        bit = 0;
    } else {
        uint16_t *source =
                &framebuffers.current_block[current_block.y + y_copy_offset];
        uint8_t source_offset = 15 - current_block.x - x_copy_offset;
        bit = utils_bit_is_set(source, source_offset);
    }

    uint16_t *destination = &framebuffer_copy[y_offset + 1];
    uint8_t destination_offset = 15 - current_block.x - x_offset;
    utils_bit_set_to(destination, destination_offset, bit);
}

static void blocks_JSTZL_rotate(void) {
    if (current_block.y == 0) {
        return;
    }

#define COPY_SIZE 3
    uint16_t framebuffer_copy[COPY_SIZE] = {};
    for (int8_t i = -1; i < COPY_SIZE - 1; i++) {
        for (int8_t j = -1; j < COPY_SIZE - 1; j++) {
            blocks_JSTZL_swap_bits(i, j, framebuffer_copy);
        }
    }

    if (current_block.y == ARRAY_SIZE(framebuffers.current_block) - 1
        && framebuffer_copy[COPY_SIZE] != 0) {
        return;
    }
    for (uint8_t i = 0; i < COPY_SIZE; i++) {
        if (framebuffer_copy[i] & WALLS) {
            return;
        }
        if (framebuffer_copy[i]
            & framebuffers.old_blocks[current_block.y - 1 + i]) {
            return;
        }
    }

    memcpy(&framebuffers.current_block[current_block.y - 1], framebuffer_copy,
           COPY_SIZE * sizeof(uint16_t));

#undef COPY_SIZE
}

static void blocks_I_swap_bits(int8_t start_x,
                               int8_t start_y,
                               uint16_t *framebuffer_copy,
                               size_t size) {
    for (uint8_t x = 0; x < size; x++) {
        for (uint8_t y = 0; y < size; y++) {
            uint8_t bit =
                    utils_bit_is_set(&framebuffers.current_block[start_y + y],
                                     15 - start_x - x);
            utils_bit_set_to(&framebuffer_copy[x], 15 - (start_x + 3 - y), bit);
        }
    }
}

static void blocks_I_rotate(void) {
    enum { STATE_U, STATE_D, STATE_L, STATE_R } i_block_state;
    uint16_t framebuffer_copy[4] = {};

    struct {
        uint8_t x;
        uint8_t y;
    } start;

    if (current_block.y == 0) {
        return;
    }

    if (framebuffers.current_block[current_block.y - 1] == 0x0000) {
        if (utils_bit_is_set(&framebuffers.current_block[current_block.y],
                             15 - (current_block.x - 2))) {
            i_block_state = STATE_D;
            start.x = current_block.x - 2;
            start.y = current_block.y - 2;
        } else {
            i_block_state = STATE_U;
            start.x = current_block.x - 1;
            start.y = current_block.y - 1;
        }
    } else {
        if (current_block.y + 2 > ARRAY_SIZE(framebuffers.current_block) - 1) {
            return;
        }
        if (utils_bit_is_set(&framebuffers.current_block[current_block.y + 2],
                             15 - (current_block.x))) {
            i_block_state = STATE_R;
            start.x = current_block.x - 2;
            start.y = current_block.y - 1;
        } else {
            i_block_state = STATE_L;
            start.x = current_block.x - 1;
            start.y = current_block.y - 2;
        }
    }

    if (current_block.y == ARRAY_SIZE(framebuffers.current_block) - 2
        && i_block_state == STATE_U) {
        return;
    }
    if (current_block.y == ARRAY_SIZE(framebuffers.current_block) - 1
        && i_block_state == STATE_D) {
        return;
    }

    blocks_I_swap_bits(start.x, start.y, framebuffer_copy,
                       ARRAY_SIZE(framebuffer_copy));

    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffer_copy); i++) {
        if (framebuffer_copy[i] & framebuffers.old_blocks[start.y + i]) {
            return;
        }
        if (framebuffer_copy[i] & WALLS) {
            return;
        }
    }

    memcpy(&framebuffers.current_block[start.y], framebuffer_copy,
           sizeof(framebuffer_copy));
    switch (i_block_state) {
    case STATE_D:
        current_block.x--;
        break;
    case STATE_U:
        current_block.x++;
        break;
    case STATE_R:
        current_block.y++;
        break;
    case STATE_L:
        current_block.y--;
        break;
    default:
        break;
    }
}

static void tetris_right_button_action() {
    if (!is_space_right()) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.current_block); i++) {
        framebuffers.current_block[i] = framebuffers.current_block[i] >> 1;
    }

    current_block.x++;
}

static void tetris_left_button_action() {
    if (!is_space_left()) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(framebuffers.current_block); i++) {
        framebuffers.current_block[i] = framebuffers.current_block[i] << 1;
    }
    current_block.x--;
}

static void tetris_up_button_action() {
    switch (current_block.block) {
    case BLOCK_TYPE_O:
        break;
    case BLOCK_TYPE_J:
    case BLOCK_TYPE_S:
    case BLOCK_TYPE_T:
    case BLOCK_TYPE_Z:
    case BLOCK_TYPE_L:
        blocks_JSTZL_rotate();
        break;
    case BLOCK_TYPE_I:
        blocks_I_rotate();
        break;
    default:
        break;
    }
}

static void tetris_down_button_action() {
    block_move_down();
    periodic_elapsed_ms = 0;
}

static bool tetris_periodic_action(uint32_t delay_ms) {
    periodic_elapsed_ms += delay_ms;
    uint16_t timer_indicator = points_counter < 30 ? points_counter : 30;
    if (periodic_elapsed_ms < 500 - 15 * timer_indicator) {
        return false;
    }

    periodic_elapsed_ms = 0;
    return block_move_down();
}

static void tetris_update_gametoy_framebuffer(uint16_t *gametoy_framebuffer) {
    memset(gametoy_framebuffer, 0, GAMETOY_DISPLAY_SIZE * sizeof(uint16_t));
    for (uint8_t i = 0; i < GAMETOY_DISPLAY_SIZE; i++) {
        if (i >= 1 && i < 1 + ARRAY_SIZE(framebuffers.points)) {
            gametoy_framebuffer[i] |= framebuffers.points[i - 1];
        }
        if (i >= 3 && i < 3 + ARRAY_SIZE(framebuffers.next_block)) {
            gametoy_framebuffer[i] |= framebuffers.next_block[i - 3];
        }
        if (i == 7) {
            gametoy_framebuffer[i] = 0xffff;
        }
        if (i >= 8 && i < 8 + ARRAY_SIZE(framebuffers.old_blocks)) {
            gametoy_framebuffer[i] |= WALLS;
            gametoy_framebuffer[i] |= framebuffers.old_blocks[i - 8];
            gametoy_framebuffer[i] |= framebuffers.current_block[i - 8];
        }
    }
}

static void tetris_initialize(void) {
    update_points_framebuffer();
    block_generate_new();
    block_generate_next();
}

static uint8_t *tetris_game_animation(void) {
    // i was too lazy to create an animation algorithm
    static uint8_t counter;

    if (counter == 0) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01110000;
        framebuffers.animation[3] = 0b00100000;
        framebuffers.animation[4] = 0b00000101;
        framebuffers.animation[5] = 0b10000111;
        framebuffers.animation[6] = 0b10001111;
        framebuffers.animation[7] = 0b11011111;
    } else if (counter == 1) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b00000000;
        framebuffers.animation[3] = 0b01110000;
        framebuffers.animation[4] = 0b00100101;
        framebuffers.animation[5] = 0b10000111;
        framebuffers.animation[6] = 0b10001111;
        framebuffers.animation[7] = 0b11011111;
    } else if (counter == 2) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b00000000;
        framebuffers.animation[3] = 0b00000000;
        framebuffers.animation[4] = 0b01110101;
        framebuffers.animation[5] = 0b10100111;
        framebuffers.animation[6] = 0b10001111;
        framebuffers.animation[7] = 0b11011111;
    } else if (counter == 3) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b00000000;
        framebuffers.animation[3] = 0b00000000;
        framebuffers.animation[4] = 0b00000101;
        framebuffers.animation[5] = 0b11110111;
        framebuffers.animation[6] = 0b10101111;
        framebuffers.animation[7] = 0b11011111;
    } else if (counter == 4) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b00000000;
        framebuffers.animation[3] = 0b00000000;
        framebuffers.animation[4] = 0b00000101;
        framebuffers.animation[5] = 0b10000111;
        framebuffers.animation[6] = 0b11111111;
        framebuffers.animation[7] = 0b11111111;
    }

    counter++;
    if (counter == 5) {
        counter = 0;
    }

    return framebuffers.animation;
}

static gametoy_actions_t tetris_actions = {
        .right_button_action = &tetris_right_button_action,
        .left_button_action = &tetris_left_button_action,
        .up_button_action = &tetris_up_button_action,
        .down_button_action = &tetris_down_button_action,
        .periodic_action = &tetris_periodic_action,
        .update_gametoy_framebuffer = &tetris_update_gametoy_framebuffer,
        .initialize = &tetris_initialize};

void tetris_game_install() {
    gametoy_game_install(&tetris_actions, GAME_TYPE_TETRIS);
    welcome_screen_animation_install(tetris_game_animation, GAME_TYPE_TETRIS);
}
