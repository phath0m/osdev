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
#include <sys/devno.h>
#include <sys/string.h>
#include <sys/types.h>

static int rtc_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos);
    
struct cdev rtc_device = {
    .name       =   "rtc",
    .mode       =   0600,
    .majorno    =   DEV_MAJOR_RTC,
    .minorno    =   0,
    .close      =   NULL,
    .init       =   NULL,
    .ioctl      =   NULL,
    .isatty     =   NULL,
    .open       =   NULL,
    .read       =   rtc_read,
    .write      =   NULL,
    .state      =   NULL
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
    while (cmos_update_in_progress());

    uint8_t second = get_rtc_register(0x00);
    uint8_t minute = get_rtc_register(0x02);
    uint8_t hour = get_rtc_register(0x04);
    uint8_t day = get_rtc_register(0x07);
    uint8_t month = get_rtc_register(0x08);
    uint8_t year = get_rtc_register(0x09);

    uint8_t regb = get_rtc_register(0x0B);

    if (!(regb & 0x04)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }

    int actual_year = 2000 + year;
    int years_since_epoch = actual_year - 1970;
    int leap_years = years_since_epoch / 4;
    int non_leap_years = years_since_epoch - leap_years;

    bool is_leap_year = actual_year % 4 == 0;

    time_t epoch = 0;

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
rtc_read(struct cdev *dev, char *buf, size_t nbyte, uint64_t pos)
{
    if (nbyte != 4) {
        return -1;
    }
    
    time_t last_time;

    do {
        last_time = get_cmos_time();
    } while (get_cmos_time() != last_time);

    memcpy(buf, &last_time, 4);

    return 4;
}
