#include "mouse.h"
#include "terminal.h"

#define MOUSE_DATA_PORT       0x60
#define MOUSE_STATUS_PORT     0x64
#define MOUSE_COMMAND_PORT    0x64

#define SCREEN_WIDTH_PIXELS   640
#define SCREEN_HEIGHT_PIXELS  400

static int mouse_x = 320;
static int mouse_y = 200;

static int old_cell_x = -1;
static int old_cell_y = -1;

static unsigned short old_cell = 0;

static int left_button = 0;
static int mouse_has_wheel = 0;


static void wait_write(void)
{
    unsigned char status;

    do
    {
        __asm__ volatile (
            "inb %1, %0"
            : "=a"(status)
            : "Nd"(MOUSE_STATUS_PORT)
        );
    }
    while (status & 2);
}


static void wait_read(void)
{
    unsigned char status;

    do
    {
        __asm__ volatile (
            "inb %1, %0"
            : "=a"(status)
            : "Nd"(MOUSE_STATUS_PORT)
        );
    }
    while (!(status & 1));
}


static void command(unsigned char value)
{
    wait_write();

    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(MOUSE_COMMAND_PORT)
    );
}


static void mouse_write(unsigned char value)
{
    command(0xD4);

    wait_write();

    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(MOUSE_DATA_PORT)
    );
}


static unsigned char mouse_read(void)
{
    unsigned char value;

    wait_read();

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(MOUSE_DATA_PORT)
    );

    return value;
}


static int mouse_data_available(void)
{
    unsigned char status;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(status)
        : "Nd"(MOUSE_STATUS_PORT)
    );

    if ((status & 1) && (status & 0x20))
        return 1;

    return 0;
}


static void hide_cursor(void)
{
    volatile unsigned short *vga =
        (volatile unsigned short *)0xB8000;

    if (old_cell_x >= 0 && old_cell_y >= 0)
    {
        vga[old_cell_y * 80 + old_cell_x] =
            old_cell;
    }

    old_cell_x = -1;
    old_cell_y = -1;
}


static void draw_cursor(void)
{
    volatile unsigned short *vga =
        (volatile unsigned short *)0xB8000;

    int cell_x = mouse_x / 8;
    int cell_y = mouse_y / 16;

    if (cell_x >= 80)
        cell_x = 79;

    if (cell_y >= 25)
        cell_y = 24;

    /*
     * Don't draw over the scrollbar itself.
     */
    if (cell_x == 79)
        return;

    old_cell_x = cell_x;
    old_cell_y = cell_y;

    old_cell = vga[cell_y * 80 + cell_x];

    /*
     * Invert the character so it looks like a cursor.
     */
    vga[cell_y * 80 + cell_x] =
        (old_cell & 0x0F00) |
        ((old_cell & 0x00FF) ^ 0x00FF);
}


void mouse_initialize(void)
{
    unsigned char id;

    /*
     * Enable the auxiliary mouse device.
     */
    command(0xA8);

    /*
     * Enable mouse interrupts in the controller.
     */
    command(0x20);

    {
        unsigned char status = mouse_read();

        status |= 2;

        command(0x60);

        wait_write();

        __asm__ volatile (
            "outb %0, %1"
            :
            : "a"(status), "Nd"(MOUSE_DATA_PORT)
        );
    }

    /*
     * Reset mouse.
     */
    mouse_write(0xFF);
    mouse_read();
    mouse_read();
    mouse_read();

    /*
     * Enable mouse.
     */
    mouse_write(0xF4);
    mouse_read();

    /*
     * Try to enable wheel support.
     */
    mouse_write(0xF3);
    mouse_read();

    mouse_write(200);
    mouse_read();

    mouse_write(0xF3);
    mouse_read();

    mouse_write(100);
    mouse_read();

    mouse_write(0xF3);
    mouse_read();

    mouse_write(80);
    mouse_read();

    mouse_write(0xF2);
    mouse_read();

    id = mouse_read();

    if (id == 3)
        mouse_has_wheel = 1;

    mouse_x = 320;
    mouse_y = 200;
}


void mouse_poll(void)
{
    static unsigned char packet[4];
    static int packet_index = 0;

    int dx;
    int dy;
    int wheel;
    int new_left;

    if (!mouse_data_available())
        return;

    packet[packet_index++] = mouse_read();

    if (packet_index < (mouse_has_wheel ? 4 : 3))
        return;

    packet_index = 0;

    /*
     * First packet byte must have bit 3 set.
     */
    if (!(packet[0] & 0x08))
        return;

    dx = (signed char)packet[1];
    dy = (signed char)packet[2];

    new_left = packet[0] & 1;

    mouse_x += dx;
    mouse_y -= dy;

    if (mouse_x < 0)
        mouse_x = 0;

    if (mouse_y < 0)
        mouse_y = 0;

    if (mouse_x >= SCREEN_WIDTH_PIXELS)
        mouse_x = SCREEN_WIDTH_PIXELS - 1;

    if (mouse_y >= SCREEN_HEIGHT_PIXELS)
        mouse_y = SCREEN_HEIGHT_PIXELS - 1;

    /*
     * Remove previous cursor before terminal redraw.
     */
    hide_cursor();

    /*
     * Mouse wheel.
     */
    if (mouse_has_wheel)
    {
        wheel = (signed char)packet[3];

        if (wheel > 0)
            terminal_scroll_up();

        if (wheel < 0)
            terminal_scroll_down();
    }

    /*
     * Clicking/dragging the scrollbar.
     */
    if (new_left && !left_button)
{
    /* X button area: top-right 24 pixels */
    if (mouse_x >= 616 && mouse_y < 16)
    {
        terminal_write("\nBatOS shutting down...\n");

        while (1)
        {
            __asm__ volatile ("hlt");
        }
    }

    /* Scrollbar */
    if (mouse_x >= 632)
    {
        int cell_y = mouse_y / 16;
        terminal_scroll_to_position(cell_y);
    }
}
    left_button = new_left;

    draw_cursor();
}