#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TITLE_BAR_ROWS 1
#define HISTORY_SIZE 500

static volatile unsigned short *const VGA_MEMORY =
    (volatile unsigned short *)0xB8000;

static unsigned short history[HISTORY_SIZE][VGA_WIDTH - 1];

static int total_lines = 1;
static int cursor_line = 0;
static int cursor_column = 0;
static int view_line = 0;

static unsigned char terminal_color = 0x07;


static unsigned short make_cell(char c)
{
    return ((unsigned short)terminal_color << 8) |
           (unsigned char)c;
}


static void clear_history_line(int line)
{
    int column;

    for (column = 0; column < VGA_WIDTH - 1; column++)
    {
        history[line][column] = make_cell(' ');
    }
}


static void draw_scrollbar(void)
{
    int row;
    int bar_size;
    int bar_position;

    for (row = 0; row < VGA_HEIGHT; row++)
    {
        VGA_MEMORY[row * VGA_WIDTH + 79] =
            ((unsigned short)0x08 << 8) | ' ';
    }

    if (total_lines <= VGA_HEIGHT)
        return;

    bar_size =
        (VGA_HEIGHT * VGA_HEIGHT) / total_lines;

    if (bar_size < 1)
        bar_size = 1;

    bar_position =
        (view_line * (VGA_HEIGHT - bar_size)) /
        (total_lines - VGA_HEIGHT);

    for (row = bar_position;
         row < bar_position + bar_size && row < VGA_HEIGHT;
         row++)
    {
        VGA_MEMORY[row * VGA_WIDTH + 79] =
            ((unsigned short)0x70 << 8) | ' ';
    }
}

static void draw_title_bar(void)
{
    int column;

    for (column = 0; column < VGA_WIDTH; column++)
    {
        VGA_MEMORY[column] =
            ((unsigned short)0x1F << 8) | ' ';
    }

    VGA_MEMORY[2] =
        ((unsigned short)0x1F << 8) | 'B';
    VGA_MEMORY[3] =
        ((unsigned short)0x1F << 8) | 'a';
    VGA_MEMORY[4] =
        ((unsigned short)0x1F << 8) | 't';
    VGA_MEMORY[5] =
        ((unsigned short)0x1F << 8) | 'O';
    VGA_MEMORY[6] =
        ((unsigned short)0x1F << 8) | 'S';

    VGA_MEMORY[78] =
        ((unsigned short)0x4F << 8) | 'X';
}


static void draw_screen(void)
{
    int row;
    int column;
    int source;

    for (row = 0; row < VGA_HEIGHT; row++)
    {
        source = view_line + row;

        for (column = 0; column < VGA_WIDTH - 1; column++)
        {
            if (source < total_lines)
            {
                VGA_MEMORY[row * VGA_WIDTH + column] =
                    history[source][column];
            }
            else
            {
                VGA_MEMORY[row * VGA_WIDTH + column] =
                    make_cell(' ');
            }
        }
    }

    draw_scrollbar();
    draw_title_bar();
}


static void add_line(void)
{
    int row;
    int column;

    if (total_lines < HISTORY_SIZE)
    {
        clear_history_line(total_lines);
        total_lines++;
        cursor_line++;
    }
    else
    {
        for (row = 1; row < HISTORY_SIZE; row++)
        {
            for (column = 0; column < VGA_WIDTH - 1; column++)
            {
                history[row - 1][column] =
                    history[row][column];
            }
        }

        clear_history_line(HISTORY_SIZE - 1);

        cursor_line = HISTORY_SIZE - 1;
    }

    cursor_column = 0;

    if (total_lines > VGA_HEIGHT)
        view_line = total_lines - VGA_HEIGHT;
    else
        view_line = 0;

    draw_screen();
}


void terminal_initialize(void)
{
    int row;

    terminal_color = 0x07;
    total_lines = 1;
    cursor_line = 0;
    cursor_column = 0;
    view_line = 0;

    for (row = 0; row < HISTORY_SIZE; row++)
    {
        clear_history_line(row);
    }

    terminal_close_button();
    draw_screen();
}


void terminal_clear(void)
{
    int row;

    total_lines = 1;
    cursor_line = 0;
    cursor_column = 0;
    view_line = 0;

    for (row = 0; row < HISTORY_SIZE; row++)
    {
        clear_history_line(row);
    }

    draw_screen();
}


void terminal_setcolor(unsigned char color)
{
    terminal_color = color;
    draw_screen();
}


void terminal_putchar(char c)
{
    if (c == '\n')
    {
        add_line();
        return;
    }

    if (cursor_column >= VGA_WIDTH - 1)
    {
        add_line();
    }

    history[cursor_line][cursor_column] =
        make_cell(c);

    cursor_column++;

    if (total_lines > VGA_HEIGHT)
        view_line = total_lines - VGA_HEIGHT;
    else
        view_line = 0;

    draw_screen();
}


void terminal_write(const char *str)
{
    int i = 0;

    while (str[i] != '\0')
    {
        terminal_putchar(str[i]);
        i++;
    }
}


void terminal_backspace(void)
{
    if (cursor_column == 0)
        return;

    cursor_column--;

    history[cursor_line][cursor_column] =
        make_cell(' ');

    draw_screen();
}


void terminal_scroll_up(void)
{
    if (view_line > 0)
    {
        view_line--;
        draw_screen();
    }
}


void terminal_scroll_down(void)
{
    int maximum;

    maximum = total_lines - VGA_HEIGHT;

    if (maximum < 0)
        maximum = 0;

    if (view_line < maximum)
    {
        view_line++;
        draw_screen();
    }
}


void terminal_scroll_page_up(void)
{
    view_line -= VGA_HEIGHT;

    if (view_line < 0)
        view_line = 0;

    draw_screen();
}


void terminal_scroll_page_down(void)
{
    int maximum;

    maximum = total_lines - VGA_HEIGHT;

    if (maximum < 0)
        maximum = 0;

    view_line += VGA_HEIGHT;

    if (view_line > maximum)
        view_line = maximum;

    draw_screen();
}


/*
 * Mouse gives us a Y position from 0-24.
 * Convert that position into a history position.
 */
void terminal_scroll_to_position(int y)
{
    int maximum;
    int new_position;

    if (total_lines <= VGA_HEIGHT)
        return;

    if (y < 0)
        y = 0;

    if (y >= VGA_HEIGHT)
        y = VGA_HEIGHT - 1;

    maximum = total_lines - VGA_HEIGHT;

    new_position =
        (y * maximum) / (VGA_HEIGHT - 1);

    view_line = new_position;

    draw_screen();
}

void terminal_close_button(void)
{
    VGA_MEMORY[0 * VGA_WIDTH + 78] =
        ((unsigned short)0x4F << 8) | 'X';
}