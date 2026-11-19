// real led controling app
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <gpiod.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GPIO1_ADDR  "4804c000.gpio" // GPIO controller on the AM335x
#define FIRST_LINE  21 // USR0 = GPIO1_21 ... USR3 = GPIO1_24

static volatile sig_atomic_t stop;
static void on_signal(int sig)
{
    (void)sig;
    stop = 1;
}

// find the /dev/gpiochipN that belongs to the GPIO1 controller
static int find_gpio1_chip(char *out, size_t len)
{
    for (int i = 0; i < 8; i++)
    {
        char link[PATH_MAX], real[PATH_MAX];
        snprintf(link, sizeof link, "/sys/bus/gpio/devices/gpiochip%d", i);

        if(realpath(link, real) && strstr(real, GPIO1_ADDR))
        {
            snprintf(out, len, "/dev/gpiochip%d", i);
            return 0;
        }
    }
    return -1;
}

static struct gpiod_line_request *request_line(const char* chip_path, unsigned int offset)
{
    struct gpiod_chip *chip = NULL;
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *lcfg = NULL;
    struct gpiod_request_config *rcfg = NULL;
    struct gpiod_line_request *req = NULL;
    
    chip = gpiod_chip_open(chip_path);
    if(!chip) { perror(chip_path); return NULL; }

    settings = gpiod_line_settings_new();
    lcfg = gpiod_line_config_new();
    rcfg = gpiod_request_config_new();

    if(settings && lcfg && rcfg)
    {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
        gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

        if(!gpiod_line_config_add_line_settings(lcfg, &offset, 1, settings))
        {
            gpiod_request_config_set_consumer(rcfg, "ledcontrol");
            req = gpiod_chip_request_lines(chip, rcfg, lcfg);

            if(!req) perror("request lines(is the pin already in use?)");
        }
        else
        {
            perror("add line settings");
        }
    }
    else
    {
        perror("alloc");
    }
    gpiod_request_config_free(rcfg);
    gpiod_line_config_free(lcfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    return req;
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "usage: %s <0-3> <on|blink [times]>\n", argv[0]);
        return 1;
    }

    int led = atoi(argv[1]);
    if(led < 0 || led > 3)
    {
        fprintf(stderr, "LED must be 0-3\n");
        return 1;
    }

    char chip_path[64];
    if(find_gpio1_chip(chip_path, sizeof chip_path))
    {
        fprintf(stderr, "could not find the GPIO1 controller");
        return 1;
    }

    unsigned int offset = FIRST_LINE + led;
    struct gpiod_line_request *req = request_line(chip_path, offset);
    if(!req) return 1;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    int ret = 0;
    if(!strcmp(argv[2], "on")) 
    {
        gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
        while(!stop)        // the line is released when we exit
            pause();
    }
    //else if(!strcmp(argv[2], "off")) return led_set(n, 0) ? 1 : 0;
    else if(!strcmp(argv[2], "blink"))
    {
        int times = argc > 3 ? atoi(argv[3]) : 10;
        for (int i = 0; i < times && !stop; i++)
        {
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE); 
            usleep(500000);
            gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE); 
            usleep(500000);
        }
    }
    else
    {
        fprintf(stderr, "unknown command '%s'\n", argv[2]);
        ret = 1;
    }
    
    return ret;
}
