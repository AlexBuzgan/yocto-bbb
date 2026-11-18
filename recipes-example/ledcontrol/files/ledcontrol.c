#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>

#ifdef SMOKETEST

#define FILE_PATH   "/dev/my_device"
#define SIZE        256

char buf[SIZE];

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        // open the file in read mode
        int fd = open(FILE_PATH, O_RDONLY);
        if (fd < 0) { perror("open"); return 1; }

        ssize_t n; // number of bytes copied
        // read all strings in the character device driver
        while((n = read(fd, buf, sizeof(buf))) != 0)
        {
            if(n < 0)
            {
                if(errno == EINTR)  continue;
                perror("read");
                close(fd);
                return 1;
            }
            fwrite(buf, 1, n, stdout); // print the read data
        }

        close(fd);
    }
    else
    {
        /* open the file in write mode */
        int fd = open(FILE_PATH, O_WRONLY);
        if (fd < 0) { perror("open"); return 1; }

        // send every string in the argument to the character device driver
        for(int i = 1; i < argc; i++)
        {
            write(fd, argv[i], strlen(argv[i]));
        }

        close(fd);
    }
    
    return 0;
}

#else // SMOKETEST

// real led controling app
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define GPIO_ROOT   "/sys/class/gpio"
#define GPIO1_ADDR  "4804c000.gpio" // GPIO controller on the AM335x
#define FIRST_LINE  21 // USR0 = GPIO1_21 ... USR3 = GPIO1_24

static int write_str(const char* path, const char *val)
{
    FILE *f = fopen(path, "w");

    if (!f) { perror(path); return -1; }
    if(fputs(val, f) == EOF) { perror(path); fclose(f); return -1; }
    return fclose(f);
}

// find the sysfs base number of the GPIO1 chip by its HW address
static int find_gpio1_base(void)
{
    DIR *d = opendir(GPIO_ROOT);
    struct dirent *e;
    int base = -1;

    if(!d) { perror(GPIO_ROOT); return -1; }
    while (e = readdir(d))
    {
        char link[PATH_MAX], real[PATH_MAX], bpath[PATH_MAX];
        if(strncmp(e->d_name, "gpiochip", 8) != 0) continue;
        snprintf(link, sizeof link, GPIO_ROOT "/%s/device", e->d_name);
        
        if(!realpath(link, real) || !strstr(real, GPIO1_ADDR)) continue;
        snprintf(bpath, sizeof bpath, GPIO_ROOT "/%s/base", e->d_name);

        FILE *f = fopen(bpath, "r");
        if(f) { if(fscanf(f, "%d", &base) != 1) base = -1; fclose(f); }
        break;
    }
    closedir(d);
    return base;
}

static int led_open(int base, int led)
{
    int n = base + FIRST_LINE + led;
    char path[PATH_MAX], num[16];

    snprintf(path, sizeof path, GPIO_ROOT "/gpio%d", n);
    struct stat st;
    if(stat(path, &st) != 0) // not exported yet
    {
        snprintf(num, sizeof num, "%d", n);
        if(write_str(GPIO_ROOT "/export", num) != 0) return -1;
    }
    snprintf(path, sizeof path, GPIO_ROOT "/gpio%d/direction", n);
    if(write_str(path, "out") != 0) return -1;

    return n;
}

static int led_set(int n, int on)
{
    char path[PATH_MAX];
    snprintf(path, sizeof path, GPIO_ROOT "/gpio%d/value", n);
    return write_str(path, on ? "1" : "0");
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "usage: %s <0-3> <on|off|blink [times]>\n", argv[0]);
        return 1;
    }

    int led = atoi(argv[1]);
    if(led < 0 || led > 3)
    {
        fprintf(stderr, "LED must be 0-3\n");
        return 1;
    }

    int base = find_gpio1_base();
    if(base < 0)
    {
        fprintf(stderr, "could not find the GPIO1 controller");
        return 1;
    }

    int n = led_open(base, led);
    if (n < 0) return 1;

    if(!strcmp(argv[2], "on")) return led_set(n, 1) ? 1: 0;
    else if(!strcmp(argv[2], "off")) return led_set(n, 0) ? 1 : 0;
    else if(!strcmp(argv[2], "blink"))
    {
        int times = argc > 3 ? atoi(argv[3]) : 10;
        for (int i = 0; i < times; i++)
        {
            led_set(n, 1); usleep(500000);
            led_set(n, 0); usleep(500000);
        }
        return 0;
    }
    fprintf(stderr, "unknown command '%s'\n", argv[2]);
    return 1;
}

#endif // SMOKETEST