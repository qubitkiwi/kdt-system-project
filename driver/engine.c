#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#define BUF_SIZE	1024
/* left leds */
#define TOY_GPIO_OUTPUT_LEFT_1 17
#define TOY_GPIO_OUTPUT_LEFT_2 27
#define TOY_GPIO_OUTPUT_LEFT_3 22
#define TOY_GPIO_OUTPUT_LEFT_4 5
/* right leds */
#define TOY_GPIO_OUTPUT_RIGHT_1 6
#define TOY_GPIO_OUTPUT_RIGHT_2 26
#define TOY_GPIO_OUTPUT_RIGHT_3 23
#define TOY_GPIO_OUTPUT_RIGHT_4 24
#define TOY_GPIO_INPUT 16

#define MAX_TIMEOUT_MSEC	3000

#define MOTOR_1_SET_SPEED _IOW('w', '1', int32_t *)
#define MOTOR_2_SET_SPEED _IOW('w', '2', int32_t *)

#define DRIVER_NAME "engine_driver"
#define DRIVER_CLASS "engine_class"
#define ON  1
#define OFF 0

#define MOTOR_1_LED_LEFT_ON() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_1, 1);	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_2, 1);	\
} while (0)

#define MOTOR_1_LED_LEFT_OFF() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_1, 0);	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_2, 0);	\
} while (0)

#define MOTOR_1_LED_RIGHT_ON() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_3, 1);	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_4, 1);	\
} while (0)

#define MOTOR_1_LED_RIGHT_OFF() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_3, 0);	\
	gpio_set_value(TOY_GPIO_OUTPUT_LEFT_4, 0);	\
} while (0)

#define MOTOR_2_LED_LEFT_ON() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_1, 1);	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_2, 1);	\
} while (0)

#define MOTOR_2_LED_LEFT_OFF() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_1, 0);	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_2, 0);	\
} while (0)

#define MOTOR_2_LED_RIGHT_ON() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_3, 1);	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_4, 1);	\
} while (0)

#define MOTOR_2_LED_RIGHT_OFF() 	\
do {	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_3, 0);	\
	gpio_set_value(TOY_GPIO_OUTPUT_RIGHT_4, 0);	\
} while (0)

#define SPEED_MAX 2000000000L
#define SPEED_MIN 20000L
#define SPEED_RESOLUTION (int) ((SPEED_MAX - SPEED_MIN) / 100)

static dev_t toy_dev;
static struct class *toy_class;
static struct cdev toy_device;
static unsigned int button_irq;

static struct hrtimer motor_1_hrtimer;
static struct hrtimer motor_2_hrtimer;

static int motor_1_toggle;
static int motor_2_toggle;
static int motor_1_speed = 50;
static int motor_2_speed = 50;

static int register_gpio_output(int gpio_num);

enum hrtimer_restart motor_1_timer_callback(struct hrtimer *timer)
{
	motor_1_toggle = motor_1_toggle ? 0 : 1;

	if (motor_1_toggle) {
		MOTOR_1_LED_LEFT_ON();
		MOTOR_1_LED_RIGHT_OFF();
	} else {
		MOTOR_1_LED_LEFT_OFF();
		MOTOR_1_LED_RIGHT_ON();
	}
	hrtimer_forward_now(timer, ktime_set(0, SPEED_MIN + motor_1_speed*SPEED_RESOLUTION));
	return HRTIMER_RESTART;
}

enum hrtimer_restart motor_2_timer_callback(struct hrtimer *timer)
{
	motor_2_toggle = motor_2_toggle ? 0 : 1;

	if (motor_2_toggle) {
		MOTOR_2_LED_LEFT_ON();
		MOTOR_2_LED_RIGHT_OFF();
	} else {
		MOTOR_2_LED_LEFT_OFF();
		MOTOR_2_LED_RIGHT_ON();
	}
	hrtimer_forward_now(timer, ktime_set(0, SPEED_MIN + motor_2_speed*SPEED_RESOLUTION));
	return HRTIMER_RESTART;
}

static ssize_t toy_driver_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{

	return BUF_SIZE;
}

ssize_t toy_driver_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	pr_info("engine_driver write: done\n");

	return count;
}

static int toy_driver_open(struct inode *device_file, struct file *instance)
{
	pr_info("engine_driver open\n");
	return 0;
}

static int toy_driver_close(struct inode *device_file, struct file *instance)
{
	pr_info("engine_driver close\n");
	return 0;
}

static long int toy_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	switch (cmd) {
		case MOTOR_1_SET_SPEED:
			pr_info("MOTOR_1: %d\n", *(int *)arg);
			motor_1_speed = *(int *)arg;
			break;
		case MOTOR_2_SET_SPEED:
			pr_info("MOTOR_2: %d\n", *(int *)arg);
			motor_2_speed = *(int *)arg;
			break;
		default:
			break;
	}
	return EINVAL;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = toy_driver_open,
	.release = toy_driver_close,
	.read = toy_driver_read,
	.write = toy_driver_write,
	.unlocked_ioctl = toy_ioctl
};

static int register_gpio_output(int gpio_num)
{
	char name[80];

	snprintf(name, sizeof(name), "toy-gpio-%d", gpio_num);

	if (gpio_request(gpio_num, name)) {
		pr_info("Can not allocate GPIO \n");
		return -1;
	}

	if(gpio_direction_output(gpio_num, 0)) {
		pr_info("Can not set GPIO  to output!\n");
		return -1;
	}

	return 0;
}

static irq_handler_t gpio_irq_emergency_stop_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	MOTOR_1_LED_RIGHT_OFF();
	MOTOR_1_LED_LEFT_OFF();
	MOTOR_2_LED_RIGHT_OFF();
	MOTOR_2_LED_LEFT_OFF();

	hrtimer_cancel(&motor_1_hrtimer);
	hrtimer_cancel(&motor_2_hrtimer);
	pr_info("emergency stop\n");
	return (irq_handler_t) IRQ_HANDLED;
}


static int __init toy_module_init(void)
{
	ktime_t ktime1, ktime2;

	if (alloc_chrdev_region(&toy_dev, 0, 1, DRIVER_NAME) < 0) {
		pr_info("Device Nr. could not be allocated!\n");
		return -1;
	}

	if ((toy_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		pr_info("Device class can not be created!\n");
		goto cerror;
	}

	if (device_create(toy_class, NULL, toy_dev, NULL, DRIVER_NAME) == NULL) {
		pr_info("Can not create device file!\n");
		goto device_error;
	}

	cdev_init(&toy_device, &fops);

	if (cdev_add(&toy_device, toy_dev, 1) == -1) {
		pr_info("Registering of device to kernel failed!\n");
		goto reg_error;
	}

	register_gpio_output(TOY_GPIO_OUTPUT_LEFT_1);
	register_gpio_output(TOY_GPIO_OUTPUT_LEFT_2);
	register_gpio_output(TOY_GPIO_OUTPUT_LEFT_3);
	register_gpio_output(TOY_GPIO_OUTPUT_LEFT_4);
	register_gpio_output(TOY_GPIO_OUTPUT_RIGHT_1);
	register_gpio_output(TOY_GPIO_OUTPUT_RIGHT_2);
	register_gpio_output(TOY_GPIO_OUTPUT_RIGHT_3);
	register_gpio_output(TOY_GPIO_OUTPUT_RIGHT_4);

	if(gpio_request(TOY_GPIO_INPUT, "toy-gpio-16")) {
		pr_info("Can not allocate GPIO 16\n");
		goto gpio_17_error;
	}

	if(gpio_direction_input(TOY_GPIO_INPUT)) {
		pr_info("Can not set GPIO 16 to input!\n");
		goto gpio_16_error;
	}

	button_irq = gpio_to_irq(TOY_GPIO_INPUT);

	/* 채터링도 방지하자!! */
	gpio_set_debounce(TOY_GPIO_INPUT, 300);

	// 여기에 GPIO 인터럽트 핸들러 등록
	if (request_irq(button_irq, (irq_handler_t) gpio_irq_emergency_stop_handler, IRQF_TRIGGER_FALLING, "kdt_gpio_irq_signal", NULL) != 0) {
		pr_err("Error! Can not request interrupt nr: %d\n", button_irq);
		return -1;
	}

	// 여기에 타이머 2개 등록
	ktime1 = ktime_set(0, motor_2_speed*SPEED_RESOLUTION);
	hrtimer_init(&motor_1_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	motor_1_hrtimer.function = &motor_1_timer_callback;
	hrtimer_start(&motor_1_hrtimer, ktime1, HRTIMER_MODE_REL);

	ktime2 = ktime_set(0, motor_2_speed*SPEED_RESOLUTION);
	hrtimer_init(&motor_2_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	motor_2_hrtimer.function = &motor_2_timer_callback;
	hrtimer_start(&motor_2_hrtimer, ktime2, HRTIMER_MODE_REL);
	return 0;

gpio_16_error:
	gpio_free(TOY_GPIO_INPUT);
gpio_17_error:
	gpio_free(17);
reg_error:
	device_destroy(toy_class, toy_dev);
device_error:
	class_destroy(toy_class);
cerror:
	unregister_chrdev_region(toy_dev, 1);
	return -1;
}

static void __exit toy_module_exit(void)
{
	free_irq(button_irq, NULL);
	hrtimer_cancel(&motor_1_hrtimer);
	hrtimer_cancel(&motor_2_hrtimer);

	gpio_set_value(TOY_GPIO_INPUT, 0);
	gpio_free(TOY_GPIO_INPUT);
	gpio_free(TOY_GPIO_OUTPUT_LEFT_1);
	gpio_free(TOY_GPIO_OUTPUT_LEFT_2);
	gpio_free(TOY_GPIO_OUTPUT_LEFT_3);
	gpio_free(TOY_GPIO_OUTPUT_LEFT_4);
	gpio_free(TOY_GPIO_OUTPUT_RIGHT_1);
	gpio_free(TOY_GPIO_OUTPUT_RIGHT_2);
	gpio_free(TOY_GPIO_OUTPUT_RIGHT_3);
	gpio_free(TOY_GPIO_OUTPUT_RIGHT_4);
	cdev_del(&toy_device);
	device_destroy(toy_class, toy_dev);
	class_destroy(toy_class);
	unregister_chrdev_region(toy_dev, 1);
	pr_info("engine module exit\n");
}

module_init(toy_module_init);
module_exit(toy_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TOY <toy@toy.com>");
MODULE_DESCRIPTION("toy engine module");
MODULE_VERSION("1.0.0");
