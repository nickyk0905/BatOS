#ifndef TERMINAL_H
#define TERMINAL_H

void terminal_initialize(void);
void terminal_setcolor(unsigned char color);
void terminal_putchar(char c);
void terminal_write(const char *str);
void terminal_backspace(void);
void terminal_clear(void);

void terminal_scroll_up(void);
void terminal_scroll_down(void);
void terminal_scroll_page_up(void);
void terminal_scroll_page_down(void);
void terminal_scroll_to_position(int y);
void terminal_close_button(void);

#endif