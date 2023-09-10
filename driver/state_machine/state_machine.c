#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define BUF_SIZE	512
#define WAIT_QUEUE_WAIT 0
#define WAIT_QUEUE_KEY 1
#define WAIT_QUEUE_NEXT 2
#define WAIT_QUEUE_EXIT 3

#define LED1 17
#define LED2 27
#define LED3 22
#define LED4 5

#define LED5 6
#define LED6 26
#define LED7 23
#define LED8 24

#define GPIO_INPUT 16

#define DRIVER_NAME "gpio_driver"
#define DRIVER_CLASS "gpio_class"

static char kernel_write_buffer[BUF_SIZE];
static dev_t st_dev;
static struct class *st_class;
static struct cdev st_device;

static struct task_struct *wait_thread;
int wait_queue_flag = WAIT_QUEUE_WAIT;
unsigned int button_irq;

DECLARE_WAIT_QUEUE_HEAD(wait_queue);


static const int LED_LIST[] = {
    LED1,
    LED2,
    LED3,
    LED4,
    LED5,
    LED6,
    LED7,
    LED8
};

static const int LED_LIST_LEN = sizeof(LED_LIST)/sizeof(LED_LIST[0]);

void set_bit_led(unsigned char led)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (led & (1 << i))
			gpio_set_value(LED_LIST[i], 1);
		else
			gpio_set_value(LED_LIST[i], 0);
	}
}

// state_machine_example_1

int state_machine_example_1(void)
{
	int state = 0;
	unsigned long hard_delay_timer_count;

	pr_info("state machine 1\n");

	set_bit_led(0x00);
	wait_queue_flag = WAIT_QUEUE_WAIT;

	while(1) {
		switch (state) {
			case 0 :
				if (gpio_get_value(GPIO_INPUT)) {
					hard_delay_timer_count = jiffies; /* Timer Start */
					state = 1;
				}
				break;
			case 1 :
				if (!gpio_get_value(GPIO_INPUT)) {
					set_bit_led(0x0F);
					state = 0;
				} else if (jiffies_to_msecs(jiffies - hard_delay_timer_count) >= 1000) {
					state = 2;
				}
				break;
			case 2 :
				if (!gpio_get_value(GPIO_INPUT)) {
					set_bit_led(0xF0);
					state = 0;
				}
				break;
			default :
				set_bit_led(0x00);
				state = 0;
				break;
		}
		usleep_range(1000, 1001);
		if (wait_queue_flag == WAIT_QUEUE_EXIT || wait_queue_flag == WAIT_QUEUE_NEXT) {
			return wait_queue_flag;
		}
	}
}

// state_machine_example_2

#define NUM_STATE	3
#define NUM_INPUT	3

enum { 
	SW_ON, 
	SW_OFF, 
	T_OUT 
};

enum { 
	ACTION_NOP, 
	ACTION_LEFT_ON, 
	ACTION_RIGHT_ON, 
	ACTION_TS 
};

struct state_machine {
	int next_state;
	int action;
};

struct state_machine example_2_state_machine[NUM_STATE][NUM_INPUT] = {
	{	/* State 0 */
		{ 1, ACTION_TS  }, 	/* input SW_ON */
		{ 0, ACTION_NOP }, 	/* input SW_OFF */
		{ 0, ACTION_NOP }  	/* input T_OUT */
	},
	{	/* State 1 */
		{ 1, ACTION_NOP },		/* input SW_ON */
		{ 0, ACTION_LEFT_ON }, /* input SW_OFF */
		{ 2, ACTION_NOP }		/* input T_OUT */
	},
	{	/* State 2 */
		{ 2, ACTION_NOP },		/* input SW_ON */
		{ 0, ACTION_RIGHT_ON },/* input SW_OFF */
		{ 2, ACTION_NOP }		/* input T_OUT */
	}
};

int state_machine_example_2(void)
{
	int state;
	int input;
	unsigned long hard_delay_timer_count;

	pr_info("state machine 2\n");

	set_bit_led(0x00);
	wait_queue_flag = WAIT_QUEUE_WAIT;
	hard_delay_timer_count = jiffies; /* Timer Start */
	state = 0;
loop:
	/* 1: Generate Input (Event) */
	if ((state == 1) && (jiffies_to_msecs(jiffies - hard_delay_timer_count) >= 1000))
		input = T_OUT;
	else if (gpio_get_value(GPIO_INPUT))
		input = SW_ON;
	else
		input = SW_OFF;

	/* 2: Do action */
	switch (example_2_state_machine[state][input].action) {
		case ACTION_TS :
			hard_delay_timer_count = jiffies; /* Timer Start */
			break;
		case ACTION_LEFT_ON :
			pr_info("<Short>\n");
			set_bit_led(0x0F);
			break;
		case ACTION_RIGHT_ON :
			pr_info("<Long>\n");
			set_bit_led(0xF0);
			break;
		default :
			break;
	}

	/* 2: Set Next State */
	state = example_2_state_machine[state][input].next_state;
	usleep_range(1000, 1001);
	if (wait_queue_flag == WAIT_QUEUE_EXIT || wait_queue_flag == WAIT_QUEUE_NEXT) {
		return wait_queue_flag;
	}

	goto loop;
}

/// state_machine_example_3

#define CLICK_LONG_TIME 1000
#define DOUBLE_CLICK_TIME 500

void(*now_state)(void);
unsigned long state3_timer;
unsigned char LED_STATE;

void idle_state(void);

void click_sl(void)
{
	if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("click_sl\n");
		// 왼쪽 led 6개 켜짐
		LED_STATE = 0b0011111;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	}
}

void click_ss(void)
{
	if (jiffies_to_msecs(jiffies - state3_timer) >= CLICK_LONG_TIME) {
		now_state = &click_sl;
	} else if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("click_ss\n");
		// 차례대로 꺼짐	
		LED_STATE = LED_STATE >> 1;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	}
}

void click_s(void)
{
	if (jiffies_to_msecs(jiffies - state3_timer) >= DOUBLE_CLICK_TIME) {
		pr_info("click_s\n");
		// 왼쪽부터 하나씩 차례로 켜짐		
		LED_STATE = LED_STATE << 1 | 0x1;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	} else if (gpio_get_value(GPIO_INPUT)) {
		state3_timer = jiffies;
		now_state = &click_ss;
	}
}

void click_ll(void)
{
	if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("click_ll\n");
		// 모든 led 꺼짐	
		LED_STATE = 0x00;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	}
}

void click_ls(void)
{
	if (jiffies_to_msecs(jiffies - state3_timer) >= CLICK_LONG_TIME) {
		now_state = &click_ll;
	} else if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("click_ls\n");
		// 모든 led 켜짐	
		LED_STATE = 0xff;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	}
	
}

void click_l(void)
{
	if (jiffies_to_msecs(jiffies - state3_timer) >= DOUBLE_CLICK_TIME) {
		pr_info("click_l\n");
		// 왼쪽 led 4개 켜짐
		LED_STATE = 0b00001111;
		set_bit_led(LED_STATE);
		now_state = &idle_state;
	} else if (gpio_get_value(GPIO_INPUT)) {
		state3_timer = jiffies;
		now_state = &click_ls;
	}
}

void click_l_off_wait(void)
{
	if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("click_l_off_wait\n");
		state3_timer = jiffies;
		now_state = &click_l;
	}
}

void check_sl(void)
{
	if (jiffies_to_msecs(jiffies - state3_timer) >= CLICK_LONG_TIME) {
		pr_info("check_sl\n");
		now_state = &click_l_off_wait;
	} else if (!gpio_get_value(GPIO_INPUT)) {
		pr_info("check_sl\n");
		state3_timer = jiffies;
		now_state = &click_s;
	}
}


void idle_state(void)
{
	if (gpio_get_value(GPIO_INPUT)) {
		pr_info("idle_state\n");
		state3_timer = jiffies;
		now_state = &check_sl;
	}
}


int state_machine_example_3(void)
{
	pr_info("state machine 3\n");

	wait_queue_flag = WAIT_QUEUE_WAIT;
	now_state = &idle_state;
	LED_STATE = 0;
	state3_timer = 0;

	while (1) {
		now_state();

		usleep_range(100, 101);
		if (wait_queue_flag == WAIT_QUEUE_EXIT || wait_queue_flag == WAIT_QUEUE_NEXT) {
			return wait_queue_flag;
		}
	}
	pr_info("exit 3\n");

	return 0;
}




static int st_kthread(void *unused)
{
	while (1) {
		if (state_machine_example_1() == WAIT_QUEUE_EXIT)
			break;
		if (state_machine_example_2() == WAIT_QUEUE_EXIT)
			break;
		if (state_machine_example_3() == WAIT_QUEUE_EXIT)
			break;
	}
	do_exit(0);
	return 0;
}





ssize_t st_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	if (copy_from_user(kernel_write_buffer, buf, count)) {
		pr_err("write: error\n");
	}

	switch (kernel_write_buffer[0]) {
		case 0:
			wait_queue_flag = WAIT_QUEUE_WAIT;
			pr_info("set WAIT_QUEUE_WAIT!\n");
			break;
		case 1:
			wait_queue_flag = WAIT_QUEUE_KEY;
			wake_up_interruptible(&wait_queue);
			pr_info("set WAIT_QUEUE_KEY!\n");
			break;
		case 2:
			wait_queue_flag = WAIT_QUEUE_NEXT;
			wake_up_interruptible(&wait_queue);
			pr_info("set WAIT_QUEUE_EXIT!\n");
			break;
		default:
			pr_info("Invalid Input!\n");
			break;
	}

	pr_info("write: done\n");

	return count;
}

static ssize_t st_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	return BUF_SIZE;
}

static int st_open(struct inode *device_file, struct file *instance)
{
	pr_info("open\n");
	return 0;
}

static int st_close(struct inode *device_file, struct file *instance)
{
	pr_info("close\n");
	return 0;
}



static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = st_open,
	.release = st_close,
	.read = st_read,
	.write = st_write
};


static int register_gpio_output(int gpio_num)
{
	char name[80];

	snprintf(name, sizeof(name), "gpio-%d", gpio_num);

	if (gpio_request(gpio_num, name)) {
		pr_info("Can not allocate GPIO %d\n", gpio_num);
		return -1;
	}

	if(gpio_direction_output(gpio_num, 0)) {
		pr_info("Can not set GPIO to output!\n");
		return -1;
	}

	return 0;
}

static int __init ST_module_init(void)
{
    int i;
	pr_info("ST_module_init\n");

	if (alloc_chrdev_region(&st_dev, 0, 1, DRIVER_NAME) < 0) {
		pr_info("Device Nr. could not be allocated!\n");
		return -1;
	}

	pr_info("할당 받은 Major = %d Minor = %d \n", MAJOR(st_dev), MINOR(st_dev));
	st_class = class_create(THIS_MODULE, DRIVER_CLASS);
	if (st_class == NULL) {
		pr_info("Device class can not be created!\n");
		goto cerror;
	}

	if (device_create(st_class, NULL, st_dev, NULL, DRIVER_NAME) == NULL) {
		pr_info("Can not create device file!\n");
		goto device_error;
	}

	cdev_init(&st_device, &fops);

	if (cdev_add(&st_device, st_dev, 1) == -1) {
		pr_info("Registering of device to kernel failed!\n");
		goto reg_error;
	}

    for (i = 0; i < LED_LIST_LEN; i++) {
        if (register_gpio_output(LED_LIST[i]) != 0)
			goto gpio_out_error;
    }

	if(gpio_request(GPIO_INPUT, "gpio-16")) {
		pr_info("Can not allocate GPIO 16\n");
		goto gpio_out_error;
	}

	if(gpio_direction_input(GPIO_INPUT)) {
		pr_info("Can not set GPIO 16 to input!\n");
		goto gpio_input_error;
	}

	wait_thread = kthread_create(st_kthread, NULL, "state machine thread");
	if (wait_thread) {
		pr_info("Thread created successfully\n");
		wake_up_process(wait_thread);
	} else
		pr_info("Thread creation failed\n");


	return 0;

gpio_input_error:
	gpio_free(16);
gpio_out_error:
    for (; i >= 0 ; i--) {
        gpio_free(LED_LIST[i]);
    }
reg_error:
	device_destroy(st_class, st_dev);
device_error:
	class_destroy(st_class);
cerror:
	unregister_chrdev_region(st_dev, 1);
	return -1;
}

static void __exit ST_module_exit(void)
{
	int i;
	wait_queue_flag = WAIT_QUEUE_EXIT;
	wake_up_interruptible(&wait_queue);
	gpio_set_value(16, 0);
	gpio_free(16);
    for (i = 0; i < LED_LIST_LEN; i++) {
        gpio_free(LED_LIST[i]);
    }
	cdev_del(&st_device);
	device_destroy(st_class, st_dev);
	class_destroy(st_class);
	unregister_chrdev_region(st_dev, 1);
	pr_info("state machine exit\n");
}


module_init(ST_module_init);
module_exit(ST_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("asong1230@gmail.com");
MODULE_DESCRIPTION("state machine");
MODULE_VERSION("1.0.0");