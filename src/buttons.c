#include <avr/interrupt.h>
#include <avr/io.h>

#include <util/atomic.h>

#include "avrtos/avrtos_delay.h"

#include "buttons.h"

#define DEBOUNCE_VALUE_US (uint64_t) 150 * 1000

#define PC0_PUSHED !(PINC & (1 << PC0))
#define PC1_PUSHED !(PINC & (1 << PC1))
#define PC2_PUSHED !(PINC & (1 << PC2))
#define PC3_PUSHED !(PINC & (1 << PC3))

#define RIGHT_BUTTON_PUSHED PC0_PUSHED
#define LEFT_BUTTON_PUSHED PC2_PUSHED
#define UP_BUTTON_PUSHED PC1_PUSHED
#define DOWN_BUTTON_PUSHED PC3_PUSHED

typedef struct {
    uint64_t timestamp;
    bool pushed;
} button_state_t;

static struct {
    button_state_t right;
    button_state_t left;
    button_state_t up;
    button_state_t down;
} buttons_state;

void buttons_init() {
    PCICR |= _BV(PCIE1);

    PCMSK1 |= _BV(PCINT8);
    PCMSK1 |= _BV(PCINT9);
    PCMSK1 |= _BV(PCINT10);
    PCMSK1 |= _BV(PCINT11);
}

#define BUTTONS_PUSHED_DEFINE(Direction)                \
    bool buttons_##Direction##_pushed() {               \
        bool ret = false;                               \
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {             \
            if (buttons_state.Direction.pushed) {       \
                ret = true;                             \
                buttons_state.Direction.pushed = false; \
            }                                           \
        }                                               \
        return ret;                                     \
    }

BUTTONS_PUSHED_DEFINE(right);
BUTTONS_PUSHED_DEFINE(left);
BUTTONS_PUSHED_DEFINE(up);
BUTTONS_PUSHED_DEFINE(down);

#undef BUTTONS_PUSHED_DEFINE

ISR(PCINT1_vect) {
    uint64_t current_us = _avrtos_delay_get_microseconds();

#define BUTTON_DEBOUNCE_DEFINE(Direction, ButtonPushed)          \
    do {                                                         \
        if (ButtonPushed && !buttons_state.Direction.pushed) {   \
            if ((current_us - buttons_state.Direction.timestamp) \
                > DEBOUNCE_VALUE_US) {                           \
                buttons_state.Direction.pushed = true;           \
                buttons_state.Direction.timestamp = current_us;  \
            }                                                    \
        }                                                        \
    } while (0)

    BUTTON_DEBOUNCE_DEFINE(right, RIGHT_BUTTON_PUSHED);
    BUTTON_DEBOUNCE_DEFINE(left, LEFT_BUTTON_PUSHED);
    BUTTON_DEBOUNCE_DEFINE(up, UP_BUTTON_PUSHED);
    BUTTON_DEBOUNCE_DEFINE(down, DOWN_BUTTON_PUSHED);

#undef BUTTON_DEBOUNCE_DEFINE
}
