#include <sys/types.h>
#include <sys/i686/portio.h>

#define CMOS_ADDRESS    0x70
#define CMOS_DATA       0x71

static int
cmos_update_in_progress()
{
    io_write_byte(CMOS_ADDRESS, 0x0A);

    return (io_read_byte(CMOS_DATA) & 0x80);
}

static uint8_t
get_rtc_register(int reg)
{
    io_write_byte(CMOS_ADDRESS, reg);

    return io_read_byte(CMOS_DATA);
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
    epoch += second;    
    
    return epoch;
}

time_t
time(time_t *second)
{
    time_t last_time;

    do {
        last_time = get_cmos_time();
    } while (get_cmos_time() != last_time);

    return last_time;
}
