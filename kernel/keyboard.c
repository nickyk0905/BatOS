#include "keyboard.h"
#include "terminal.h"

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

static const char keyboard_map[] =
{
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*',
    0,  ' '
};


void keyboard_initialize(void)
{
}


static int keyboard_data_available(void)
{
    unsigned char status;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(status)
        : "Nd"(KEYBOARD_STATUS_PORT)
    );

    /*
     * Bit 0 = data available
     * Bit 5 = mouse data
     */
    if ((status & 1) && !(status & 0x20))
        return 1;

    return 0;
}


static unsigned char keyboard_read(void)
{
    unsigned char key;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(key)
        : "Nd"(KEYBOARD_DATA_PORT)
    );

    return key;
}


int keyboard_try_get_key(void)
{
    static int extended = 0;
    static int shift = 0;
    unsigned char key;

    if (!keyboard_data_available())
        return KEY_NONE;

    key = keyboard_read();

    /* Left Shift pressed */
    if (key == 0x2A)
    {
        shift = 1;
        return KEY_NONE;
    }

    /* Right Shift pressed */
    if (key == 0x36)
    {
        shift = 1;
        return KEY_NONE;
    }

    /* Left Shift released */
    if (key == 0xAA)
    {
        shift = 0;
        return KEY_NONE;
    }

    /* Right Shift released */
    if (key == 0xB6)
    {
        shift = 0;
        return KEY_NONE;
    }
    /*
     * Ignore key release.
     */
    if (key & 0x80)
    {
        extended = 0;
        return KEY_NONE;
    }

    /*
     * Extended keyboard key.
     */
    if (key == 0xE0)
    {
        extended = 1;
        return KEY_NONE;
    }

    if (extended)
    {
        extended = 0;

        if (key == 0x48)
            return KEY_UP;

        if (key == 0x50)
            return KEY_DOWN;

        if (key == 0x49)
            return KEY_PAGE_UP;

        if (key == 0x51)
            return KEY_PAGE_DOWN;

        return KEY_NONE;
    }

    if (key < sizeof(keyboard_map))
    {
        if (keyboard_map[key] != 0)
        {
            /* Shift + . = > */
            if (shift && key == 0x34)
                return '>';

            return keyboard_map[key];
        }
    }
    return KEY_NONE;
}