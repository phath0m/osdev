/*
 * rtc.c - RTC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <machine/portio.h>
#include <sys/cdev.h>
#include <sys/device.h>
#include <sys/devno.h>
#include <sys/string.h>
#include <sys/types.h>

static int rtc_attach(struct driver *driver, struct device *dev);
static int rtc_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
    
struct driver rtc_driver = {
    .attach     =   rtc_attach,
    .deattach   =   NULL,
    .probe      =   NULL
};

#define CMOS_ADDRESS    0x70
#define CMOS_DATA       0x71

static int
cmos_update_in_progress()
{
    io_write8(CMOS_ADDRESS, 0x0A);

    return (io_read8(CMOS_DATA) & 0x80);
}

static uint8_t
get_rtc_register(int reg)
{
    io_write8(CMOS_ADDRESS, reg);

    return io_read8(CMOS_DATA);
}

static int seconds_per_month_lut[] = {
    0, 2678400, 5097600, 7776000, 10368000, 13046400, 15638400, 18316800, 20995200, 23587200, 26265600, 28857600
};

static int seconds_per_month_leap_lut[] = {
    0, 2678400, 5184000, 7862400, 10454400, 13132800, 15724800, 18403200, 21081600, 23673600, 26352000, 28944000
};

static time_t
get_cmos_time()
{
    bool is_leap_year;
    int actual_year;
    int years_since_epoch;
    int leap_years;
    int non_leap_years;

    time_t epoch;

    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t regb;

    while (cmos_update_in_progress());

    second = get_rtc_register(0x00);
    minute = get_rtc_register(0x02);
    hour = get_rtc_register(0x04);
    day = get_rtc_register(0x07);
    month = get_rtc_register(0x08);
    year = get_rtc_register(0x09);
    regb = get_rtc_register(0x0B);

    if (!(regb & 0x04)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }

    actual_year = 2000 + year;
    years_since_epoch = actual_year - 1970;
    leap_years = years_since_epoch / 4;
    non_leap_years = years_since_epoch - leap_years;

    is_leap_year = actual_year % 4 == 0;

    epoch = 0;

    epoch += leap_years * 31622400;
    epoch += non_leap_years * 31536000;
    
    if (is_leap_year) {
        epoch += seconds_per_month_lut[month - 1];
    } else {
        epoch += seconds_per_month_leap_lut[month - 1];
    }

    epoch += day * 86400;
    epoch += hour * 3600;
    epoch += minute * 60;

    return epoch;
}

static int
rtc_attach(struct driver *driver, struct device *dev)
{
    struct cdev *cdev;
    struct cdev_ops rtc_ops;

    rtc_ops = (struct cdev_ops){
        .close  = NULL,
        .init   = NULL,
        .ioctl  = NULL,
        .isatty = NULL,
        .mmap   = NULL,
        .open   = NULL,
        .read   = rtc_read,
        .write  = NULL
    };

    cdev = cdev_new("rtc", 0666, DEV_MAJOR_RTC, 0, &rtc_ops, NULL);

    if (cdev && cdev_register(cdev) == 0) {
        return 0;
    }

    return -1;
}

static int
rtc_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    time_t last_time;

    if (nbyte != 4) {
        return -1;
    }

    do {
        last_time = get_cmos_time();
    } while (get_cmos_time() != last_time);

    memcpy(buf, &last_time, 4);

    return 4;
}
