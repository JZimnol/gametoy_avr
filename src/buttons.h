#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdbool.h>

void buttons_init(void);
bool buttons_right_pushed();
bool buttons_left_pushed();
bool buttons_up_pushed();
bool buttons_down_pushed();

#endif /* BUTTONS_H_ */
