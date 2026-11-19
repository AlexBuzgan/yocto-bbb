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

int main(void)
{
    char chip_path[64];

    if(find_gpio1_chip(chip_path, sizeof chip_path))
    {
        fprintf(stderr, "could not find the GPIO1 controller");
        return 1;
    }

    unsigned int offset = FIRST_LINE;
    struct gpiod_line_request *req = request_line(chip_path, offset);
    if(!req) return 1;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    enum gpiod_line_value val = GPIOD_LINE_VALUE_INACTIVE;

    while(!stop)
    {
        val = (val == GPIOD_LINE_VALUE_INACTIVE) ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
        
        if (gpiod_line_request_set_value(req, offset, val) < 0)
        {
            perror("gpiod_line_request_set_value");
            break;
        }

        usleep(500000);
    }

    gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_release(req);
    
    return 0;
}
