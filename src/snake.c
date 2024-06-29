#include <stdbool.h>
#include <string.h>

#include "gametoy.h"
#include "snake.h"
#include "utils.h"
#include "welcome_screen.h"

#define ROWS 23
#define COLS 14

static const uint16_t WALLS = 0x8001;

static enum { MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT } next_move;

static struct {
    uint16_t snake[23]; // 8-30
    uint16_t points[5]; // 1-5
    uint16_t food;
    uint8_t animation[WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE];
} framebuffers;

typedef struct {
    uint8_t x;
    uint8_t y;
} coordinates_t;

static coordinates_t snake[ROWS * COLS]; // this one takes a lot of resources
static coordinates_t food;

static uint16_t snake_len;
static uint32_t periodic_elapsed_ms;
static bool move_already_choosen;

static void food_generate_new(void);

static inline bool coordinates_equal(coordinates_t *a, coordinates_t *b) {
    return (a->x == b->x && a->y == b->y);
}

static inline void coordinates_copy(coordinates_t *dest, coordinates_t *src) {
    dest->x = src->x;
    dest->y = src->y;
}

static void update_points_framebuffer(void) {
    memset(framebuffers.points, 0, sizeof(framebuffers.points));
    gametoy_update_points_framebuffer(framebuffers.points, snake_len);
}

static void game_over(void) {
    gametoy_game_over(snake_len);
}

static bool snake_check_bit_boundaries(uint8_t row, uint8_t col) {
    if (row >= ARRAY_SIZE(framebuffers.snake)) {
        return false;
    }

    if (col >= COLS + 1 || col < 1) {
        return false;
    }

    return true;
}

static uint8_t snake_get_framebuffer(coordinates_t *field) {
    uint8_t row = field->y;
    uint8_t col = field->x;
    if (!snake_check_bit_boundaries(row, col)) {
        return false;
    }

    return utils_bit_is_set(&framebuffers.snake[row], 15 - col);
}

static uint8_t food_get_framebuffer(coordinates_t *field) {
    uint8_t row = field->y;
    uint8_t col = field->x;
    if (!snake_check_bit_boundaries(row, col)) {
        return false;
    }

    return utils_bit_is_set(&framebuffers.food, 15 - col);
}

static void snake_set_framebuffer(coordinates_t *field, uint8_t bit) {
    uint8_t row = field->y;
    uint8_t col = field->x;
    if (!snake_check_bit_boundaries(row, col)) {
        return;
    }

    utils_bit_set_to(&framebuffers.snake[row], 15 - col, bit);
}

static void food_set_framebuffer(coordinates_t *field, uint8_t bit) {
    uint8_t row = field->y;
    uint8_t col = field->x;
    if (!snake_check_bit_boundaries(row, col)) {
        return;
    }

    utils_bit_set_to(&framebuffers.food, 15 - col, bit);
}

static void snake_move(void) {
    coordinates_t new_head = {};
    coordinates_copy(&new_head, &snake[0]);

    switch (next_move) {
    case MOVE_DOWN:
        new_head.y++;
        if (new_head.y == ROWS) {
            new_head.y = 0;
        }
        break;
    case MOVE_UP:
        if (new_head.y == 0) {
            new_head.y = ROWS - 1;
        } else {
            new_head.y--;
        }
        break;
    case MOVE_RIGHT:
        if (new_head.x == COLS) {
            new_head.x = 1;
        } else {
            new_head.x++;
        }
        break;
    case MOVE_LEFT:
        if (new_head.x == 1) {
            new_head.x = COLS;
        } else {
            new_head.x--;
        }
        break;
    default:
        break;
    }

    for (uint8_t i = 0; i < snake_len - 1; i++) {
        if (coordinates_equal(&new_head, &snake[i])) {
            game_over();
        }
    }

    if (coordinates_equal(&new_head, &food)) {
        for (int16_t i = snake_len; i >= 0; i--) {
            snake[i + 1].x = snake[i].x;
            snake[i + 1].y = snake[i].y;
        }
        snake_len++;
        coordinates_copy(&snake[0], &new_head);
        update_points_framebuffer();
        food_generate_new();
        if (snake_len == ARRAY_SIZE(snake)) {
            game_over();
        }
    } else {
        for (uint16_t i = snake_len - 1; i > 0; i--) {
            coordinates_copy(&snake[i], &snake[i - 1]);
        }
        coordinates_copy(&snake[0], &new_head);
    }

    memset(&framebuffers.snake, 0, sizeof(framebuffers.snake));
    for (uint16_t i = 0; i < snake_len; i++) {
        snake_set_framebuffer(&snake[i], 1);
    }
    move_already_choosen = false;
}

static void snake_right_button_action(void) {
    if (next_move == MOVE_LEFT || move_already_choosen) {
        return;
    }
    next_move = MOVE_RIGHT;
    move_already_choosen = true;
    periodic_elapsed_ms = 0;
    snake_move();
}

static void snake_left_button_action(void) {
    if (next_move == MOVE_RIGHT || move_already_choosen) {
        return;
    }
    next_move = MOVE_LEFT;
    move_already_choosen = true;
    periodic_elapsed_ms = 0;
    snake_move();
}

static void snake_up_button_action(void) {
    if (next_move == MOVE_DOWN || move_already_choosen) {
        return;
    }
    next_move = MOVE_UP;
    move_already_choosen = true;
    periodic_elapsed_ms = 0;
    snake_move();
}

static void snake_down_button_action(void) {
    if (next_move == MOVE_UP || move_already_choosen) {
        return;
    }
    next_move = MOVE_DOWN;
    move_already_choosen = true;
    periodic_elapsed_ms = 0;
    snake_move();
}

static void food_generate_new(void) {
    coordinates_t new_food;
    uint16_t rand_val = gametoy_get_random_value() % COLS + 1; // 1-14
    while (true) {
        bool found = true;
        for (uint8_t i = rand_val; i < rand_val + ROWS; i++) {
            new_food.x = rand_val;
            new_food.y = i % ROWS;
            for (uint16_t j = 0; j < snake_len; j++) {
                if (coordinates_equal(&snake[j], &new_food)) {
                    found = false;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
        rand_val++;
        if (rand_val == COLS + 1) {
            rand_val = 1;
        }
    }

    food_set_framebuffer(&food, 0);
    coordinates_copy(&food, &new_food);
    food_set_framebuffer(&food, 1);
}

static bool food_toggle_framebuffer(uint32_t *delay_ms) {
    static uint32_t ms;
    ms += *delay_ms;

    if (ms < 450) {
        return false;
    }

    food_set_framebuffer(&food, !food_get_framebuffer(&food));

    return true;
}

static bool snake_head_toogle_framebuffer(uint32_t *delay_ms) {
    static uint32_t ms;
    ms += *delay_ms;
    if (ms < 100) {
        return false;
    }

    snake_set_framebuffer(&snake[0], !snake_get_framebuffer(&snake[0]));

    return true;
}

static bool snake_periodic_action(uint32_t delay_ms) {
    bool ret;
    ret = food_toggle_framebuffer(&delay_ms);
    ret |= snake_head_toogle_framebuffer(&delay_ms);

    periodic_elapsed_ms += delay_ms;
    if (periodic_elapsed_ms < 300) {
        return ret;
    }

    periodic_elapsed_ms = 0;
    snake_move();

    return true;
}

static void snake_update_gametoy_framebuffer(uint16_t *gametoy_framebuffer) {
    memset(gametoy_framebuffer, 0, GAMETOY_DISPLAY_SIZE * sizeof(uint16_t));

    for (uint8_t i = 0; i < GAMETOY_DISPLAY_SIZE; i++) {
        if (i >= 1 && i < 1 + ARRAY_SIZE(framebuffers.points)) {
            gametoy_framebuffer[i] |= framebuffers.points[i - 1];
        }
        if (i == 7) {
            gametoy_framebuffer[i] = 0xffff;
        }
        if (i >= 8 && i < 8 + ARRAY_SIZE(framebuffers.snake)) {
            gametoy_framebuffer[i] |= WALLS;
            gametoy_framebuffer[i] |= framebuffers.snake[i - 8];
        }
        if (i == GAMETOY_DISPLAY_SIZE - 1) {
            gametoy_framebuffer[i] = 0xffff;
        }
        if (i == food.y + 8) {
            gametoy_framebuffer[i] |= framebuffers.food;
        }
    }
}

static void snake_initialize(void) {
    snake[0].x = 2;
    snake[0].y = 2;
    snake_len = 1;
    snake_set_framebuffer(&snake[0], 1);

    food_generate_new();

    next_move = MOVE_DOWN;

    update_points_framebuffer();
}

static uint8_t *snake_game_animation(void) {
    // i was too lazy to create an animation algorithm
    static uint8_t counter;

    if (counter == 0) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01111100;
        framebuffers.animation[3] = 0b01000000;
        framebuffers.animation[4] = 0b00000000;
        framebuffers.animation[5] = 0b00000000;
        framebuffers.animation[6] = 0b00000100;
        framebuffers.animation[7] = 0b00000000;
    } else if (counter == 1) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01111000;
        framebuffers.animation[3] = 0b01000000;
        framebuffers.animation[4] = 0b01000000;
        framebuffers.animation[5] = 0b00000000;
        framebuffers.animation[6] = 0b00000100;
        framebuffers.animation[7] = 0b00000000;
    } else if (counter == 2) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01110000;
        framebuffers.animation[3] = 0b01000000;
        framebuffers.animation[4] = 0b01000000;
        framebuffers.animation[5] = 0b01000000;
        framebuffers.animation[6] = 0b00000100;
        framebuffers.animation[7] = 0b00000000;
    } else if (counter == 3) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01100000;
        framebuffers.animation[3] = 0b01000000;
        framebuffers.animation[4] = 0b01000000;
        framebuffers.animation[5] = 0b01000000;
        framebuffers.animation[6] = 0b01000100;
        framebuffers.animation[7] = 0b00000000;
    } else if (counter == 4) {
        framebuffers.animation[0] = 0b00000000;
        framebuffers.animation[1] = 0b00000000;
        framebuffers.animation[2] = 0b01000000;
        framebuffers.animation[3] = 0b01000000;
        framebuffers.animation[4] = 0b01000000;
        framebuffers.animation[5] = 0b01000000;
        framebuffers.animation[6] = 0b01100100;
        framebuffers.animation[7] = 0b00000000;
    }

    counter++;
    if (counter == 5) {
        counter = 0;
    }

    return framebuffers.animation;
}

static gametoy_actions_t snake_actions = {
        .right_button_action = &snake_right_button_action,
        .left_button_action = &snake_left_button_action,
        .up_button_action = &snake_up_button_action,
        .down_button_action = &snake_down_button_action,
        .periodic_action = &snake_periodic_action,
        .update_gametoy_framebuffer = &snake_update_gametoy_framebuffer,
        .initialize = &snake_initialize};

void snake_game_install() {
    gametoy_game_install(&snake_actions, GAME_TYPE_SNAKE);
    welcome_screen_animation_install(snake_game_animation, GAME_TYPE_SNAKE);
}
