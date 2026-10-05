#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NONE       0
#define KEY_UP         1000
#define KEY_DOWN       1001
#define KEY_PAGE_UP    1002
#define KEY_PAGE_DOWN  1003

void keyboard_initialize(void);
int keyboard_try_get_key(void);

#endif