#ifndef WELCOME_SCREEN_H_
#define WELCOME_SCREEN_H_

#include <inttypes.h>

#define WELCOME_SCREEN_ANIMATION_FRAMEBUFFER_SIZE 8

typedef uint8_t *welcome_screen_get_game_animation_t(void);

void welcome_screen_install();
void welcome_screen_initialize(void);
void welcome_screen_animation_install(
        welcome_screen_get_game_animation_t *animation, game_type_t game_type);

#endif /* WELCOME_SCREEN_H_ */
