#ifndef GAMETOY_H_
#define GAMETOY_H_

#include <inttypes.h>
#include <stdbool.h>

#define GAMETOY_DISPLAY_SIZE 32

typedef enum {
    GAME_TYPE_NONE,
    GAME_TYPE_SNAKE,
    GAME_TYPE_TETRIS,
    _GAME_TYPE_COUNT
} game_type_t;

typedef void gametoy_right_button_action_t(void);
typedef void gametoy_left_button_action_t(void);
typedef void gametoy_up_button_action_t(void);
typedef void gametoy_down_button_action_t(void);
typedef bool gametoy_periodic_action_t(uint32_t delay_ms);
typedef void
gametoy_update_gametoy_framebuffer_t(uint16_t *gametoy_framebuffer);
typedef void gametoy_initialize_t(void);

typedef struct {
    gametoy_right_button_action_t *right_button_action;
    gametoy_left_button_action_t *left_button_action;
    gametoy_up_button_action_t *up_button_action;
    gametoy_down_button_action_t *down_button_action;
    gametoy_periodic_action_t *periodic_action;
    gametoy_update_gametoy_framebuffer_t *update_gametoy_framebuffer;
    gametoy_initialize_t *initialize;
} gametoy_actions_t;

void gametoy_start(void);
uint16_t gametoy_get_random_value(void);
void gametoy_update_points_framebuffer(uint16_t *framebuffer, uint16_t points);
void gametoy_game_over(uint16_t points);
void gametoy_game_install(gametoy_actions_t *actions, game_type_t game_type);
void gametoy_game_select(game_type_t game);
void gametoy_game_run(void);

#endif /* GAMETOY_H_ */
