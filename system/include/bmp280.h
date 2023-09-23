#ifndef BMP280_DEV_H_
#define BMP280_DEV_H_
#include <linux/ioctl.h>

#define BMP_PATH "/dev/bmp280_spi_driver"

struct bmp280_data {
    int temp;
    unsigned int press;
};

#define IOCTL_MAGIC         'G'
#define GET_ID              _IOR(IOCTL_MAGIC, 1 , int)
#define BMP280_INIT         _IOR(IOCTL_MAGIC, 2 , int)
#define GET_DATA            _IOR(IOCTL_MAGIC, 3 , struct bmp280_data)

enum osrs {
    OVERSAMPLING_SKIP   = 0b000,
    OVERSAMPLING_X1     = 0b001,
    OVERSAMPLING_X2     = 0b010,
    OVERSAMPLING_X4     = 0b011,
    OVERSAMPLING_X8     = 0b100,
    OVERSAMPLING_X16    = 0b101
};

enum mode {
    SLEEP  = 0b00,
    FPRCED = 0b01,
    NORMAL = 0b11
};

enum t_sb {
    STANDBY_05MS    = 0b000,
    STANDBY_62MS    = 0b001,
    STANDBY_125MS   = 0b010,
    STANDBY_250MS   = 0b011,
    STANDBY_500MS   = 0b100,
    STANDBY_1000MS  = 0b101,
    STANDBY_2000MS  = 0b110,
    STANDBY_4000MS  = 0b111
};

enum iir_filter {
    FILTER_OFF  = 0b0000,
    FILTER_2    = 0b0001,
    FILTER_4    = 0b0010,
    FILTER_8    = 0b0100,
    FILTER_16   = 0b1000
};

#endif