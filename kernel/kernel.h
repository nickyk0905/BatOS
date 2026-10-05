#ifndef TERMINAL_H
#define TERMINAL_H

void terminal_initialize(void);
void terminal_clear(void);
void terminal_putchar(char c);
void terminal_write(const char *data);
void terminal_setcolor(unsigned char color);

#endif
