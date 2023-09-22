#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define BMP_PATH "/dev/bmp280_spi_driver"

#define IOCTL_MAGIC         'G'
#define GET_ID              _IOR(IOCTL_MAGIC, 1 , int)
#define BMP280_INIT         _IOR(IOCTL_MAGIC, 2 , int)
#define GET_DATA            _IOR(IOCTL_MAGIC, 3 , struct bmp280_data)

struct bmp280_data {
    int temp;
    unsigned int press;
};

int main(void)
{
    int dev_fd, bmp_id;
    struct bmp280_data sensor_data;

    dev_fd = open(BMP_PATH, O_RDWR);
    if (dev_fd == -1) {
        perror("BMP open err");
        return -1;
    }

    ioctl(dev_fd, GET_ID, (int*) &bmp_id);
    printf("bmp280 id = %x\n", bmp_id);

    ioctl(dev_fd, BMP280_INIT, 0);

    usleep(50000);

    ioctl(dev_fd, GET_DATA, (struct bmp280_data*) &sensor_data);
    printf("tmp : %d, press : %d\n", sensor_data.temp, sensor_data.press);


    close(dev_fd);
    return 0;
}