#include "terminal.h"
#include "shell.h"
#include "fs.h"

void kernel_main(void)
{
    terminal_initialize();
    terminal_setcolor(0x0F);

    terminal_write("Welcome to Bat OS\n");

    fs_init();

    shell_start();

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}