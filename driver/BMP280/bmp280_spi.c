#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include "bmp280.h"

#define DRIVER_NAME "bmp280_spi_driver"
#define DRIVER_CLASS "bmp280_spi_class"

#define SPI_BUS_NUM 0

#define BMP280_ID 0x58

dev_t dev = 0;
static struct class *dev_class;
static struct cdev bmp280_cdev;

static struct spi_device *bmp280_spi_device;

struct spi_board_info bmp280_spi_device_info = {
	.modalias = "bmp280",
	.max_speed_hz = 1000000,
	.bus_num = SPI_BUS_NUM,
	.chip_select = 0,
	.mode = 3,
};

static int32_t sensor_value = 0;

struct calculation {
    unsigned short dig_T1;
    short dig_T2;
    short dig_T3;

    unsigned short dig_P1;
    short dig_P2;
    short dig_P3;
    short dig_P4;
    short dig_P5;
    short dig_P6;
    short dig_P7;
    short dig_P8;
    short dig_P9;
};

static struct calculation calc;
static int t_fine;

static void bmp280_calc_init(void)
{
    calc.dig_T1 = spi_w8r16(bmp280_spi_device, 0x88);
    calc.dig_T2 = spi_w8r16(bmp280_spi_device, 0x8a);
    calc.dig_T3 = spi_w8r16(bmp280_spi_device, 0x8c);

    calc.dig_P1 = spi_w8r16(bmp280_spi_device, 0x8e);
    calc.dig_P2 = spi_w8r16(bmp280_spi_device, 0x90);
    calc.dig_P3 = spi_w8r16(bmp280_spi_device, 0x92);
    calc.dig_P4 = spi_w8r16(bmp280_spi_device, 0x94);
    calc.dig_P5 = spi_w8r16(bmp280_spi_device, 0x96);
    calc.dig_P6 = spi_w8r16(bmp280_spi_device, 0x98);
    calc.dig_P7 = spi_w8r16(bmp280_spi_device, 0x9a);
    calc.dig_P8 = spi_w8r16(bmp280_spi_device, 0x9c);
    calc.dig_P9 = spi_w8r16(bmp280_spi_device, 0x9e);
    
    pr_info("dig_T %d %d %d\n", calc.dig_T1, calc.dig_T2, calc.dig_T3);
}

static void bmp280_reset(void)
{
    unsigned char buf[] = {0x60, 0xB6};
    
    spi_write(bmp280_spi_device, buf, 2);
}



static void bmp280_set_filter(enum iir_filter filter)
{
    unsigned char buf[2];

    buf[0] = 0x75;
    buf[1] = spi_w8r8(bmp280_spi_device, 0xF5) & 0b11100011;
    buf[1] |= filter << 2;

    pr_info("bmp280_set_filter : %x", buf[1]);
    spi_write(bmp280_spi_device, buf, 2);    
}

static void bmp280_set_time_standby(enum t_sb t_standby)
{   
    unsigned char buf[2];

    buf[0] = 0x75;
    buf[1] = spi_w8r8(bmp280_spi_device, 0xF5) & !0b11100000;
    buf[1] |= t_standby << 5;

    pr_info("time_standby : %x", buf[1]);
    spi_write(bmp280_spi_device, buf, 2);    
}

static void bmp280_set_config_reg(enum iir_filter filter ,enum t_sb t_standby)
{
    bmp280_set_filter(filter);
    bmp280_set_time_standby(t_standby);
}



static void bmp280_set_osrs(enum osrs osrs_t, enum osrs osrs_p)
{
    unsigned char buf[2];

    buf[0] = 0x74;
    buf[1] = spi_w8r8(bmp280_spi_device, 0xF4) & !0b11111100;
    buf[1] |= ((osrs_t << 3) | osrs_p) << 2;

    pr_info("bmp280_set_osrs : %x", buf[1]);

    spi_write(bmp280_spi_device, buf, 2);    
}

static void bmp280_set_mode(enum mode MODE)
{
    unsigned char buf[2];

    buf[0] = 0x74;
    buf[1] = spi_w8r8(bmp280_spi_device, 0xF4) & 0b11111100;
    buf[1] |= MODE;

    pr_info("mode : %x", buf[1]);

    spi_write(bmp280_spi_device, buf, 2);    
}

static void bmp280_set_ctrl_meas_reg(enum osrs osrs_t, enum osrs osrs_p, enum mode MODE)
{
    unsigned char buf[2];

    buf[0] = 0x74;
    buf[1] = (osrs_t << 5) | (osrs_p << 2) | MODE;

    pr_info("bmp280_set_ctrl_meas_reg : %x %x", buf[0], buf[1]);

    spi_write(bmp280_spi_device, buf, 2);   
}

static int bmp280_id_get(void)
{
    int id;
    id = spi_w8r8(bmp280_spi_device, 0xD0);
    return id;
}

static int bmp280_temp_get(void)
{
    int temp_xlsb, temp_lsb, temp_msb, raw_temp, var1, var2, temp;

    temp_xlsb = spi_w8r8(bmp280_spi_device, 0xFC);
    temp_lsb = spi_w8r8(bmp280_spi_device, 0xFB);
    temp_msb = spi_w8r8(bmp280_spi_device, 0xFA);

    // char tx[] = {0xFA, 0};
    // int tmp = 0;

    // spi_write_then_read(bmp280_spi_device, tx, 1, &tmp, 3);
    
    raw_temp = ((temp_msb << 16) | (temp_lsb << 8) | temp_xlsb) >> 4;

    // pr_info("T = %d %d\n", raw_temp, tmp>>4);

    var1 = ((((raw_temp >> 3) - (calc.dig_T1 << 1))) * (calc.dig_T2)) >> 11;
	var2 = (((((raw_temp >> 4) - (calc.dig_T1)) * ((raw_temp >> 4) - (calc.dig_T1))) >> 12) * (calc.dig_T3)) >> 14;
	
    t_fine = var1 + var2;
    temp = (t_fine * 5 + 128) >> 8;

    return temp;
}


static unsigned int bmp280_press_get(void)
{
    int press_msb, press_lsb, press_xlsb, raw_press;
    int var1, var2;
    unsigned int press;
    
    press_xlsb = spi_w8r8(bmp280_spi_device, 0xF9);
    press_lsb = spi_w8r8(bmp280_spi_device, 0xF8);
    press_msb = spi_w8r8(bmp280_spi_device, 0xF7);

    raw_press = ((press_msb << 16) | (press_lsb << 8) | press_xlsb) >> 4;

    
    var1 = (((int)t_fine)>>1) - (int)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int)calc.dig_P6);
    var2 = var2 + ((var1*((int)calc.dig_P5))<<1);
    var2 = (var2>>2)+(((int)calc.dig_P4)<<16);
    var1 = (((calc.dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int)calc.dig_P2) * var1)>>1))>>18;
    var1 =((((32768+var1))*((int)calc.dig_P1))>>15);
    
    if (var1 == 0) {
        return 0;
    }

    press = (((unsigned int)(((int)1048576)-raw_press)-(var2>>12)))*3125;
    if (press < 0x80000000) {
        press = (press << 1) / ((unsigned int)var1);
    } else {
        press = (press / (unsigned int)var1) * 2;
    }

    var1 = (((int)calc.dig_P9) * ((int)(((press>>3) * (press>>3))>>13)))>>12;
    var2 = (((int)(press>>2)) * ((int)calc.dig_P8))>>13;

    press = (unsigned int)((int)press + ((var1 + var2 + calc.dig_P7) >> 4));
    return press;
}



static int bmp280_open(struct inode *inode, struct file *file)
{
    pr_info("bmp280 open");
    return 0;
}

static int bmp280_release(struct inode *inode, struct file *file)
{
    pr_info("bmp280 close");
    return 0;
}

static ssize_t bmp280_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    pr_info("bmp280 read");
    return 0;
}

static ssize_t bmp280_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    pr_info("bmp280 write");
    return len;
}

static long bmp280_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct bmp280_data data = {0,0};

    switch(cmd) {
        case GET_ID:
            pr_info("GET_ID\n");
            sensor_value = bmp280_id_get();
            if( copy_to_user((int32_t*) arg, &sensor_value, sizeof(sensor_value)) ) {
                pr_err("bmp 280 id Read : Err!\n");
            }
            break;
        case BMP280_INIT:
            pr_info("BMP280_INIT\n");
            bmp280_reset();
            msleep(200);
            bmp280_calc_init();
            bmp280_set_config_reg(FILTER_16, STANDBY_500MS);
            bmp280_set_ctrl_meas_reg(OVERSAMPLING_X2, OVERSAMPLING_X16, NORMAL);
            break;
        case GET_DATA:
            pr_info("GET_DATA\n");
            data.temp = bmp280_temp_get();
            data.press = bmp280_press_get();
            if( copy_to_user((int32_t*) arg, &data, sizeof(data)) ) {
                pr_err("Data Read : Err!\n");
            }
            pr_info("tmep = %d\n", data.temp);

            break;
        default:
            pr_info("Default\n");
            break;
    }
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = bmp280_read,
    .write          = bmp280_write,
    .open           = bmp280_open,
    .unlocked_ioctl = bmp280_ioctl,
    .release        = bmp280_release,
};


static int __init bmp280_module_init (void)
{
    struct spi_master *master;

    pr_info("bmp280 module init\n");

    /*Allocating Major number*/
    if((alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME)) <0){
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&bmp280_cdev,&fops);

    /*Adding character device to the system*/
    if((cdev_add(&bmp280_cdev, dev, 1)) < 0){
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }

    /*Creating struct class*/
    if(IS_ERR(dev_class = class_create(THIS_MODULE, DRIVER_CLASS))){
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    /*Creating device*/
    if(IS_ERR(device_create(dev_class,NULL,dev,NULL,DRIVER_NAME))){
        pr_err("Cannot create the Device 1\n");
        goto r_device;
    }
    pr_info("Device Driver Insert...Done!!!\n");

    // spi init
    master = spi_busnum_to_master( bmp280_spi_device_info.bus_num );
    if( master == NULL ) {
        pr_err("SPI Master not found.\n");
        goto r_device;
    }
    
    // create a new slave device, given the master and device info
    bmp280_spi_device = spi_new_device( master, &bmp280_spi_device_info );
    if( bmp280_spi_device == NULL ) {
        pr_err("FAILED to create slave.\n");
        goto r_device;
    }
    
    // 8-bits in a word
    bmp280_spi_device->bits_per_word = 8;

    // setup the SPI slave device
    if( spi_setup(bmp280_spi_device) ) {
        pr_err("FAILED to setup slave.\n");
        goto r_spi;
    }
    
    // bmp280 init
    // if (bmp280_id_get != BMP280_ID) {
    //     pr_err("Not BMP280 id\n");
    //     goto r_spi;
    // }

    pr_info("BMP280_INIT\n");
    bmp280_reset();
    msleep(200);
    bmp280_calc_init();
    bmp280_set_config_reg(FILTER_16, STANDBY_500MS);
    bmp280_set_ctrl_meas_reg(OVERSAMPLING_X2, OVERSAMPLING_X16, NORMAL);

    return 0;

r_spi:
    spi_unregister_device( bmp280_spi_device );
r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev,1);
    return -1;
}

static void __exit bmp280_module_exit (void)
{
    spi_unregister_device( bmp280_spi_device );
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&bmp280_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("bmp280 module exit\n");
}

module_init(bmp280_module_init);
module_exit(bmp280_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<asong1230@gmail.com>");
MODULE_DESCRIPTION("bmp280 module");
MODULE_VERSION("1.0.0");