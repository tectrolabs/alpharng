/*
 * alrandom.h
 * ver. 1.0
 *
 */

/*
 * AlphaRNG device driver
 *
 * Copyright (C) 2014-2022 TectroLabs, https://tectrolabs.com
 *
 * This is a 'alrandom' kernel module that registers a character device
 * for supplying true random bytes generated by an AlphaRNG hardware
 * random number generator.
 *
 * After the module is successfully loaded by the kernel, the random bytes
 * will be available through /dev/alrandom device.
 *
 * To test, simply plug an AlphaRNG device into one of the available USB ports
 * and run the following command:
 *
 * sudo dd if=/dev/alrandom of=/dev/null bs=100000 count=10
 *
 * Module's internal status can be verified with the following command:
 * cat /proc/alrandom/info
 *
 * Please note that modules's internal status is only updated when the AlphaRNG
 * device is in use.
 *
 */
#ifndef ALRANDOM_H_
#define ALRANDOM_H_


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/completion.h>


#include <linux/tty.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/termios.h>


#define SUCCESS 0
#define DEVICE_NAME "alrandom"
#define PROC_NAME "info"
#define DRIVER_VERSION "1.0"
#define DRIVER_NAME "ALRNG"


#define WORD_SIZE_BYTES (4)
#define MIN_INPUT_NUM_WORDS (8)
#define OUT_NUM_WORDS (8)
#define NUM_CHUNKS (500)
#define RND_IN_BUFFSIZE (NUM_CHUNKS * MIN_INPUT_NUM_WORDS * WORD_SIZE_BYTES)
#define TRND_OUT_BUFFSIZE (NUM_CHUNKS * OUT_NUM_WORDS * WORD_SIZE_BYTES)
#define USB_BUFFER_SIZE (32000)
#define USB_READ_MAX_RETRY_CNT   (15)
#define USB_READ_TIMEOUT_SECS (1)

#define DEVICE_MODEL_LENGTH   (8)
#define DEVICE_SERIAL_NUM_LENGTH (15)

#define MAX_ACM_DEVICES_TO_PROBE (5)
#define ACM_DEV_NAME_LENGTH_LIMIT (386)
#define ACM_DEV_NAME_BY_ID_LENGTH_LIMIT (256)

// Max amount of entropy bytes that user can request at a time.
#define MAX_BYTES_USER_CAN_REQUEST (100000)


typedef int (*acm_readdir_t)(void *, const char *, int, loff_t, u64, unsigned);

//
// Function declarations
//
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t * offset);
static ssize_t proc_read(struct file *file, char __user *buffer, size_t length, loff_t * offset);
static int get_entropy_bytes(void);
static int rcv_rnd_bytes(void);
static void wait_for_pending_ops(void);
static int chip_read_data(char *buff, int length, int opTimeoutSecs);
static int snd_rcv_usb_data(char *snd, int sizeSnd, char *rcv, int sizeRcv, int opTimeoutSecs);
static int init_char_dev(void);
static void uninit_char_dev(void);
static int create_device(void);
static int create_proc(void);
static void remove_proc(void);
static void probe_init(void);
static void log_device_connect_message(void);
static int thread_function(void *data);
static ssize_t thread_device_read(char *buffer, size_t length);
static void clear_receive_buffer(int opTimeoutSecs);

static void test_samples(void);
static void configure_tests(void);

static void rct_initialize(void);
static void rct_restart(void);

static void apt_initialize(void);
static void apt_restart(void);

//
// ACM functions
//
static bool acm_device_probe(void);
static int acm_read(struct file *file, unsigned char *data, int size);
static int acm_full_read(unsigned char *data, int size, int *bytesTransfered);
static int acm_write(struct file *file, const unsigned char *data, int size);
static void acm_close(struct file *file);
static int acm_iterate_dir_callback(struct dir_context *ctx, const char *name, int nameLength, loff_t offset, u64 ino,
      unsigned int dType);
static int acm_readdir(const char* path, acm_readdir_t filler, void* context);
static int acm_filldir_callback(void* data, const char *name, int nameLength, loff_t offset, u64 ino, unsigned int dType);
static void acm_clean_up(void);
static bool acm_set_tty_termios_flags(void);
static bool acm_open_device(void);
static bool acm_lock_device(void);
static struct file *acm_open(const char *path, int flags);

//
// Data section
//

// Mutex for synchronization
static struct mutex dataOpLock;

// Reference to the character device
static struct cdev *cdv = NULL;

// Reference to the character device class
static struct class *dev_class = NULL;

// Reference to the proc parent directory
static struct proc_dir_entry *proc_parent_dir = NULL;

// A flag indication that the proc info can be retrieved
static bool proc_ready_to_read_flag = true;

static struct kthread_data {
   /*
    * Command sent to thread function.
    * Valid commands are:
    * 'r' - requesting entropy bytes
    * 'e' - module is unloading
    */
   char command;
   /*
    * Status returned from the thread function.
    * 0 - for successful operation
    * non zero value - error number
    */
   ssize_t status;

   struct task_struct *drv_thread;
   /*
    * A buffer for storing entropy bytes to be delivered to user space.
    */
   char k_buffer[MAX_BYTES_USER_CAN_REQUEST];
   /*
    * Actual amount of entropy bytes to be delivered which can be less than amount requested from the user space.
    * It must be <= MAX_BYTES_USER_CAN_REQUEST
    */
   size_t k_length;

   struct completion to_thread_event;
   struct completion from_thread_event;

} *threadData;

static struct ctrl_data {
   unsigned char bulk_in_buffer[USB_BUFFER_SIZE];
   unsigned char bulk_out_buffer[1];
   char statusByteHolder[1];
   bool isInitialized;
   char receiveClearBuff[1024];
} *ctrlData;

// Major and minor numbers assigned to the char device
static int major = 0;
static int minor = 0;


// Declare device operation handlers
static struct file_operations fops = {
      .owner = THIS_MODULE,
      .read = device_read};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,05,00)
static struct file_operations proc_fops = {
      .read = proc_read};
#else
static struct proc_ops proc_fops = {
      .proc_read = proc_read};
#endif

// Pointer to the random input buffer
static char *buffRndIn;

// Current index for the buffRndIn buffer
static volatile int curRndInIdx = RND_IN_BUFFSIZE;

// Pointer to the random output buffer
static char *buffTRndOut;

// Current index for the buffTRndOut buffer
static volatile int curTrngOutIdx = TRND_OUT_BUFFSIZE;

// A flag indicating when the entropy source is ready
static volatile bool isEntropySrcRdy = false;

// A flag indicating when there are pending device operations like read or write
static volatile bool isDeviceOpPending = false;

// A flag indicating when there are pending proc read operations
static volatile bool isProcOpPending = false;

// A flag indicating when there are pending USB operations like read or write
static volatile bool isUsbOpPending = false;

// A flag to signal a shutdown event
static volatile bool isShutDown = false;

// How many statistical test failures allowed per block (16000 random bytes)
static uint8_t numFailuresThreshold = 4;

// A flag for enabling debug mode
static bool debugMode = false;

// A flag to indicate if statistical tests for raw data should be disabled.
static bool disableStatisticalTests;

// How many bytes to download per sample.
static int bytesPerSample = TRND_OUT_BUFFSIZE;

// Max number of repetition count test failures encountered per data block
static uint16_t maxRctFailuresPerBlock;

// Max number of adaptive proportion test failures encountered per data block
static uint16_t maxAptFailuresPerBlock;

// Total number of repetition count test failures encountered for current device
static uint64_t totalRctFailuresForCurrentDevice;

// Total number of adaptive proportion test failures encountered for current device
static uint64_t totalAptFailuresForCurrentDevice;

// Last known device status byte
static uint8_t deviceStatusByte = 0;

// Total number of requests handled by device
static uint64_t deviceTotalRequestsHandled = 0;


//.................
// ACM related data
//.................

// Path for finding connected AlphaRNG ACM devices
static char dev_serial_by_id_path[] = "/dev/serial/by-id";

// ACM TTY context
static struct acm_context {
   char dev_name[ACM_DEV_NAME_LENGTH_LIMIT];
   char dev_name_by_id[MAX_ACM_DEVICES_TO_PROBE][ACM_DEV_NAME_BY_ID_LENGTH_LIMIT];
   int devices_found;
   struct path path;
   struct inode *inode;
   dev_t devt;
   struct tty_struct *tty;
   struct ktermios opts;
   struct file *filed;
   bool device_open;
   struct file_lock fl;
   bool device_locked;
} *acmCtxt = NULL;

struct acm_callback_context {
   struct dir_context ctx;
   acm_readdir_t filler;
   void* context;
};


//..........
// Test data
//..........

// Repetition Count Test data
static struct rct_data {
   volatile uint32_t maxRepetitions;
   volatile uint32_t curRepetitions;
   volatile uint8_t lastSample;
   volatile uint8_t statusByte;
   volatile uint8_t signature;
   volatile bool isInitialized;
   volatile uint16_t failureCount;
} rct;

// Adaptive Proportion Test data
static struct apt_data {
   volatile uint16_t windowSize;
   volatile uint16_t cutoffValue;
   volatile uint16_t curRepetitions;
   volatile uint16_t curSamples;
   volatile uint8_t statusByte;
   volatile uint8_t signature;
   volatile bool isInitialized;
   volatile uint8_t firstSample;
   volatile uint16_t cycleFailures;
} apt;

static const uint8_t maxDataBlockSizeWords = 16;

#endif /* ALRANDOM_H_ */
