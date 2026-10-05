#include "shell.h"
#include "terminal.h"
#include "keyboard.h"
#include "mouse.h"
#include "fs.h"

static unsigned int boot_total_seconds = 0;

unsigned char rtc_read(unsigned char reg)
{
    unsigned char value;

    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(reg), "Nd"((unsigned short)0x70)
    );

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"((unsigned short)0x71)
    );

    return value;
}

unsigned char bcd_to_binary(unsigned char value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

void print_2digits(unsigned char value)
{
    char text[3];

    text[0] = '0' + (value / 10);
    text[1] = '0' + (value % 10);
    text[2] = '\0';

    terminal_write(text);
}

static int string_compare(const char *a, const char *b)
{
    int i = 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}


static int string_starts_with(const char *text, const char *prefix)
{
    int i = 0;

    while (prefix[i] != '\0')
    {
        if (text[i] != prefix[i])
            return 0;

        i++;
    }

    return 1;
}


static void reboot_system(void)
{
    unsigned char good = 0x02;

    while (good & 0x02)
    {
        __asm__ volatile (
            "inb $0x64, %0"
            : "=a"(good)
        );
    }

    __asm__ volatile (
        "outb %0, $0x64"
        :
        : "a"((unsigned char)0xFE)
    );

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}

char cat_text[FS_MAX_FILE_SIZE];

void shell_start(void)
{
    char command[64];
    char cat_text[FS_MAX_FILE_SIZE];
    int index = 0;

    keyboard_initialize();
    mouse_initialize();

    {
      unsigned char boot_hour   = bcd_to_binary(rtc_read(0x04));
        unsigned char boot_minute = bcd_to_binary(rtc_read(0x02));
        unsigned char boot_second = bcd_to_binary(rtc_read(0x00));

        boot_total_seconds = (unsigned int)boot_hour * 3600
                            + (unsigned int)boot_minute * 60
                            + (unsigned int)boot_second;
    }

    terminal_write("\n> ");

    while (1)
    {
        int key;

        /*
         * Check mouse continuously.
         */
        mouse_poll();

        /*
         * Check keyboard without blocking.
         */
        key = keyboard_try_get_key();

        if (key == KEY_NONE)
            continue;

        /*
         * Scrolling.
         */
        if (key == KEY_UP)
        {
            terminal_scroll_up();
            continue;
        }

        if (key == KEY_DOWN)
        {
            terminal_scroll_down();
            continue;
        }

        if (key == KEY_PAGE_UP)
        {
            terminal_scroll_page_up();
            continue;
        }

        if (key == KEY_PAGE_DOWN)
        {
            terminal_scroll_page_down();
            continue;
        }

        /*
         * Enter.
         */
        if (key == '\n')
        {

            command[index] = '\0';

            if (string_compare(command, "help"))
            {
                terminal_write("\n");
                terminal_write("Available commands:\n");
                terminal_write("help     - show available commands\n");
                terminal_write("about    - about BatOS\n");
                terminal_write("version  - show BatOS version\n");
                terminal_write("whoami   - show current user\n");
                terminal_write("clear    - clear the screen\n");
                terminal_write("echo     - print text\n");
                terminal_write("info     - show BatOS information\n");
                terminal_write("hello    - say hello\n");
                terminal_write("pwd      - show current directory\n");
                terminal_write("ls       - list files\n");
                terminal_write("cd       - change directory\n");
                terminal_write("mkdir    - create directory\n");
                terminal_write("rmdir    - remove directory\n");
                terminal_write("touch    - create file\n");
                terminal_write("cat      - display file\n");
                terminal_write("rm       - remove file\n");
                terminal_write("cp       - copy file\n");
                terminal_write("mv       - move/rename file\n");
                terminal_write("uname    - show system information\n");
                terminal_write("mem      - show memory information\n");
                terminal_write("cpu      - show CPU information\n");
                terminal_write("history  - show command history\n");
                terminal_write("date     - show system date\n");
                terminal_write("time     - show system time\n");
                terminal_write("uptime   - show system uptime\n");
                terminal_write("reboot   - restart BatOS\n");
                terminal_write("shutdown - shut down BatOS\n");
                terminal_write("exit     - exit shell\n");
            }
            else if (string_compare(command, "about"))
            {
                terminal_write("\nBatOS - Your own operating system!\n");
            }
            else if (string_compare(command, "version"))
            {
                terminal_write("\nBatOS version 0.1\n");
            }
            else if (string_compare(command, "whoami"))
            {
                terminal_write("\nYou are the BatOS user.\n");
            }
            else if (string_compare(command, "clear"))
            {
                terminal_clear();
            }
            else if (string_compare(command, "hello"))
            {
                terminal_write("\nHello from BatOS!\n");
            }
            else if (string_compare(command, "info"))
            {
                terminal_write("\nBatOS Information\n");
                terminal_write("----------------\n");
                terminal_write("Name: BatOS\n");
                terminal_write("Version: 0.1\n");
                terminal_write("Architecture: x86\n");
                terminal_write("Status: Running\n");
            }
            else if (string_starts_with(command, "echo "))
            {
                terminal_write("\n");
                terminal_write(command + 5);
                terminal_write("\n");
            }

            /* ================= FILESYSTEM COMMANDS ================= */

            else if (string_compare(command, "pwd"))
            {
                fs_cmd_pwd();
            }
            else if (string_compare(command, "ls"))
            {
                fs_cmd_ls();
            }
            else if (string_starts_with(command, "cd "))
            {
                fs_cmd_cd(command + 3);
            }
            else if (string_starts_with(command, "mkdir "))
            {
                fs_cmd_mkdir(command + 6);
            }
            else if (string_starts_with(command, "rmdir "))
            {
                fs_cmd_rmdir(command + 6);
            }
            else if (string_starts_with(command, "touch "))
            {
                fs_cmd_touch(command + 6);
            }
            else if (string_starts_with(command, "cat >> "))
{
    char *path = command + 7;

    if (path[0] == '\0')
    {
        terminal_write("\nUsage: cat >> <file>\n");
    }
    else
    {
        terminal_write("\nEnter text: ");

        index = 0;

        while (1)
        {
            int write_key = keyboard_try_get_key();

            if (write_key == KEY_NONE)
                continue;

            if (write_key == '\n')
            {
                cat_text[index] = '\0';
                fs_cmd_cat_write(path, cat_text, 1);
                break;
            }

            if (write_key == '\b')
            {
                if (index > 0)
                {
                    index--;
                    terminal_backspace();
                }
                continue;
            }

            if (index < FS_MAX_FILE_SIZE - 1 &&
                write_key >= 32 &&
                write_key <= 126)
            {
                cat_text[index++] = (char)write_key;
                terminal_putchar((char)write_key);
            }
        }
    }
}
else if (string_starts_with(command, "cat > "))
{
    char *path = command + 6;

    if (path[0] == '\0')
    {
        terminal_write("\nUsage: cat > <file>\n");
    }
    else
    {
        terminal_write("\nEnter text: ");

        index = 0;

        while (1)
        {
            int write_key = keyboard_try_get_key();

            if (write_key == KEY_NONE)
                continue;

            if (write_key == '\n')
            {
                cat_text[index] = '\0';
                fs_cmd_cat_write(path, cat_text, 0);
                break;
            }

            if (write_key == '\b')
            {
                if (index > 0)
                {
                    index--;
                    terminal_backspace();
                }
                continue;
            }

            if (index < FS_MAX_FILE_SIZE - 1 &&
                write_key >= 32 &&
                write_key <= 126)
            {
                cat_text[index++] = (char)write_key;
                terminal_putchar((char)write_key);
            }
        }
    }
}
else if (string_starts_with(command, "cat "))
{
    fs_cmd_cat(command + 4);
}
            else if (string_starts_with(command, "rm "))
            {
                fs_cmd_rm(command + 3);
            }
            else if (string_starts_with(command, "cp "))
            {
                char src[FS_MAX_PATH_LEN];
                char dst[FS_MAX_PATH_LEN];

                const char *args = command + 3;

                int i = 0;
                int j = 0;

                while (args[i] != '\0' &&
                      args[i] != ' ' &&
                      i < FS_MAX_PATH_LEN - 1)
                {
                    src[i] = args[i];
                    i++;
                }

                src[i] = '\0';

                while (args[i] == ' ')
                    i++;

                while (args[i] != '\0' &&
                      j < FS_MAX_PATH_LEN - 1)
                {
                    dst[j] = args[i];
                    i++;
                    j++;
                }

                dst[j] = '\0';

                fs_cmd_cp(src, dst);
            }
            else if (string_starts_with(command, "mv "))
            {
                char src[FS_MAX_PATH_LEN];
                char dst[FS_MAX_PATH_LEN];

                const char *args = command + 3;

                int i = 0;
                int j = 0;

                while (args[i] != '\0' &&
                      args[i] != ' ' &&
                      i < FS_MAX_PATH_LEN - 1)
                {
                    src[i] = args[i];
                    i++;
                }

                src[i] = '\0';

                while (args[i] == ' ')
                    i++;

                while (args[i] != '\0' &&
                      j < FS_MAX_PATH_LEN - 1)
                {
                    dst[j] = args[i];
                    i++;
                    j++;
                }

                dst[j] = '\0';

                fs_cmd_mv(src, dst);
            }
            else if (string_compare(command, "uname"))
            {
                terminal_write("\nBatOS x86\n");
            }

            else if (string_compare(command, "mem"))
            {
                terminal_write("\nMemory information is not implemented yet.\n");
            }

            else if (string_compare(command, "cpu"))
            {
                terminal_write("\nCPU information is not implemented yet.\n");
            }

            else if (string_compare(command, "history"))
            {
                terminal_write("\nCommand history is not implemented yet.\n");
            }

            else if (string_compare(command, "date"))
            {
                unsigned char day;
                unsigned char month;
                unsigned char year;

                day = bcd_to_binary(rtc_read(0x07));
                month = bcd_to_binary(rtc_read(0x08));
                year = bcd_to_binary(rtc_read(0x09));

                terminal_write("\nDate: ");
                print_2digits(day);
                terminal_write("/");
                print_2digits(month);
                terminal_write("/20");
                print_2digits(year);
                terminal_write("\n");
            }            

            else if (string_compare(command, "time"))
            {
                unsigned char hour;
                unsigned char minute;
                unsigned char second;

                hour   = bcd_to_binary(rtc_read(0x04));
                minute = bcd_to_binary(rtc_read(0x02));
                second = bcd_to_binary(rtc_read(0x00));

                /* Convert UTC -> IST (UTC+5:30) */
                minute += 30;
                if (minute >= 60)
                {
                    minute -= 60;
                    hour += 1;
                }

                hour += 5;
                if (hour >= 24)
                {
                    hour -= 24;
                }

                terminal_write("\nIST Time: ");

                print_2digits(hour);
                terminal_write(":");

                print_2digits(minute);
                terminal_write(":");

                print_2digits(second);

                terminal_write("\n");
            }

            else if (string_compare(command, "uptime"))
            {
                unsigned char cur_hour   = bcd_to_binary(rtc_read(0x04));
                unsigned char cur_minute = bcd_to_binary(rtc_read(0x02));
                unsigned char cur_second = bcd_to_binary(rtc_read(0x00));

                unsigned int current_total_seconds =
                      (unsigned int)cur_hour * 3600
                    + (unsigned int)cur_minute * 60
                    + (unsigned int)cur_second;

                unsigned int elapsed_seconds;

                if (current_total_seconds >= boot_total_seconds)
                {
                    elapsed_seconds = current_total_seconds - boot_total_seconds;
                }
                else
                {
                    /* Crossed midnight since boot */
                    elapsed_seconds = (86400 - boot_total_seconds) + current_total_seconds;
                }

                {
                    unsigned char up_hour   = (unsigned char)(elapsed_seconds / 3600);
                    unsigned char up_minute = (unsigned char)((elapsed_seconds % 3600) / 60);
                    unsigned char up_second = (unsigned char)(elapsed_seconds % 60);

                    terminal_write("\nUptime: ");
                    print_2digits(up_hour);
                    terminal_write(":");
                    print_2digits(up_minute);
                    terminal_write(":");
                    print_2digits(up_second);
                    terminal_write("\n");
                }
            }

            else if (string_compare(command, "shutdown"))
            {
                terminal_write("\nShutting down BatOS...\n");

                while (1)
                {
                    __asm__ volatile ("hlt");
                }
            }

            else if (string_compare(command, "exit"))
            {
                terminal_write("\nExiting shell...\n");

                while (1)
                {
                    __asm__ volatile ("hlt");
                }
            }

            else if (string_compare(command, "reboot"))
            {
                terminal_write("\nRestarting BatOS...\n");
                reboot_system();
            }

            else if (command[0] != '\0')
            {
                terminal_write("\nUnknown command.\n");
            }

            index = 0;

            terminal_write("\n> ");
            continue;
        }

        /*
         * Backspace.
         */
        if (key == '\b')
        {
            if (index > 0)
            {
                index--;
                terminal_backspace();
            }

            continue;
        }

        /*
         * Normal character.
         */
        if (index < 63 && key >= 32 && key <= 126)
        {
            command[index] = (char)key;
            index++;

            terminal_putchar((char)key);
        }
    }
}