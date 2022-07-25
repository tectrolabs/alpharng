/*
 * alrandom.c
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
#include "alrandom.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrian Belinski");
MODULE_DESCRIPTION("A module that registers a device for supplying true random bytes generated by Hardware RNG AlphaRNG");
MODULE_VERSION(DRIVER_VERSION);

module_param(disableStatisticalTests, bool, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(disableStatisticalTests, "A flag to indicate if statistical tests for the raw data should be disabled");

module_param(bytesPerSample, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(bytesPerSample, "How many bytes to download per sample. Valid value must be between 1 and 16000. Providing this parameter may decrease performance");

module_param(debugMode, bool, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debugMode, "A flag to enable debug mode");

/**
 * A function called from a thread to read from an ACM device
 *
 * @param buffer - pointer for destination bytes
 * @param length - how many bytes to read
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static ssize_t thread_device_read(char *buffer, size_t length)
{
   ssize_t retval = SUCCESS;
   size_t act;
   size_t total;

   if (isShutDown) {
      return -ENODATA;
   }

   if (!isEntropySrcRdy) {
      // No USB device has been probed.
      // Probe for an ACM device.
      if (!acm_device_probe()) {
         // No ACM device has been found
         retval = -ENODATA;
      }
   }

   if (isEntropySrcRdy) {
      total = 0;
      do {
         retval = get_entropy_bytes();
         if (retval == SUCCESS) {
            act = TRND_OUT_BUFFSIZE - curTrngOutIdx;
            if (act > (length - total)) {
               act = (length - total);
            }
            memcpy(buffer + total, buffTRndOut + curTrngOutIdx, act);
            curTrngOutIdx += act;
            total += act;
            retval = total;
         } else {
            if (acmCtxt->device_locked) {
               acm_clean_up();
            }
            break;
         }
      } while (total < length);
      if (debugMode) {
         if (total > length) {
            pr_err("thread_device_read(): Expected %d bytes to read and actually got %d \n", (int)length, (int)total);
         }
      }
   } else {
      retval = -ENODATA;
   }

   return retval;
}

/**
 * A function to handle the device read operation from the user space
 *
 * @param file - pointer to the file structure of the caller
 * @param buffer - pointer to the buffer in the user space
 * @param length - size in bytes for the read operation
 * @param offset
 * @return greater than 0 - number of bytes actually read, otherwise the error code (a negative number)
 *
 */
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t * offset)
{
   ssize_t retval = SUCCESS;
   unsigned long waitStatus;

   if (isShutDown) {
      return -ENODATA;
   }

   if (length == 0) {
      return 0;
   }

   if (mutex_lock_killable(&dataOpLock) != SUCCESS) {
      if(debugMode) {
         pr_err("%s: device_read(): Could not lock the mutex\n", DRIVER_NAME);
      }
      return -EPERM;
   }

   isDeviceOpPending = true;

   // Create thread command
   threadData->command = 'r';
   threadData->status = -ETIMEDOUT;
   if (length > MAX_BYTES_USER_CAN_REQUEST) {
      // Limit the amount of entropy bytes that can be retrieved at a time
      threadData->k_length = MAX_BYTES_USER_CAN_REQUEST;
   } else {
      threadData->k_length = length;
   }

   if (!completion_done(&threadData->to_thread_event)) {
      complete(&threadData->to_thread_event);
   } else {
      retval = -EBUSY;
      goto return_lable;
   }

   waitStatus = wait_for_completion_timeout(&threadData->from_thread_event, msecs_to_jiffies(5000));
   if (waitStatus == 0) {
      if (debugMode) {
         pr_err("device_read(): thread timeout reached when processing request\n");
      }
      retval = -ETIMEDOUT;
   } else {
      retval = threadData->status;
      if (retval > 0 && retval > MAX_BYTES_USER_CAN_REQUEST) {
         pr_err("%s: device_read(): BUG: invalid return value %d\n", DRIVER_NAME, (int)retval);
         retval = -EFAULT;
      } else if (retval > 0 && retval <= MAX_BYTES_USER_CAN_REQUEST) {
         if (copy_to_user(buffer, threadData->k_buffer, retval)) {
            retval = -EFAULT;
         }
      }
   }

return_lable:
   isDeviceOpPending = false;
   mutex_unlock(&dataOpLock);

   return retval;
}

/**
 * A function to handle the /proc/alrandom/info read operation from user space.
 * It is to be invoked in a single user mode only, for troubleshooting purposes.
 *
 * @param file - pointer to the file structure of the caller
 * @param buffer - pointer to the buffer in the user space
 * @param length - size in bytes for the read operation
 * @param offset
 * @return greater than 0 - number of bytes actually read, otherwise the error code (a negative number)
 *
 */
static ssize_t proc_read(struct file *file, char __user *buffer, size_t length, loff_t * offset)
{
   int len = 0;
   char *msg = NULL;
   int bytesNotCopied = 0;

   if (isShutDown) {
      return -ENODATA;
   }

   if (length == 0) {
      return 0;
   }

   if (mutex_lock_killable(&dataOpLock) != SUCCESS) {
      if(debugMode) {
         pr_err("%s: proc_read(): Could not lock the mutex\n", DRIVER_NAME);
      }
      return -EPERM;
   }
   isProcOpPending = true;

   if (proc_ready_to_read_flag) {
      if (isEntropySrcRdy == false) {
         // No device has been detected
         len = 30;
         bytesNotCopied = (int)copy_to_user(buffer, "No AlphaRNG information found\n", len);
         if (bytesNotCopied != 0) {
            pr_err("%s: proc_read(): copy_to_user(): Could not copy %d bytes out of %d \n", DRIVER_NAME, bytesNotCopied, len);
         }
      } else {
         if (ctrlData->isInitialized == true) {
            // Retrieve device information and statistics
            msg = kasprintf(GFP_KERNEL,
                  "AlphaRNG statistical tests: %s\n"
                  "maximum RCT failures per block for device: %d\n"
                  "maximum APT failures per block for device: %d\n"
                  "total RCT failures for device: %llu\n"
                  "total APT failures for device: %llu\n"
                  "RCT status byte for device: %d\n"
                  "APT status byte for device: %d\n"
                  "last known device status byte: %d\n"
                  "number of requests handled by device: %llu\n"
                  ,disableStatisticalTests ? "disabled" : "enabled"
                  ,maxRctFailuresPerBlock
                  ,maxAptFailuresPerBlock
                  ,totalRctFailuresForCurrentDevice
                  ,totalAptFailuresForCurrentDevice
                  ,rct.statusByte
                  ,apt.statusByte
                  ,(int)deviceStatusByte
                  ,deviceTotalRequestsHandled);
            if (msg != NULL) {
               len = strlen(msg);
               bytesNotCopied = (int)copy_to_user(buffer, msg, len);
               if (bytesNotCopied != 0) {
                  pr_err("%s: proc_read(): copy_to_user(): Could not copy %d bytes out of %d to user space\n", DRIVER_NAME, bytesNotCopied, len);
               }
               kfree(msg);
            } else {
               pr_err("proc_read: Could not allocate memory for generating device information\n");
            }
         } else {
            // No device has been initialized
            len = 39;
            bytesNotCopied = (int)copy_to_user(buffer, "The AlphaRNG device wasn't initialized\n", len);
            if (bytesNotCopied != 0) {
               pr_err("%s: proc_read(): copy_to_user(): failed to copy %d bytes out of %d to user space\n", DRIVER_NAME, bytesNotCopied, len);
            }

         }
      }
   } else {
      // Device information and statistics already retrieved
      len = 0;
   }

   proc_ready_to_read_flag ^= true;

   isProcOpPending = false;
   mutex_unlock(&dataOpLock);

   return len;
}

/**
 * This is a thread function for handling device commands invoked from the user space.
 * For security reasons this command handling logic is executed in a dedicated kernel thread.
 *
 * @param data - a pointer to thread private data
 *
 * @return 0 when shutting down
 */
int thread_function(void *data)
{
   unsigned long waitStatus;
   struct kthread_data *thData = (struct kthread_data *)data;

   while (!isShutDown) {
      waitStatus = wait_for_completion_timeout(&threadData->to_thread_event, msecs_to_jiffies(1000));
      if (waitStatus == 0) {
         continue;
      }

      switch(thData->command) {
      case 'e':
         // Module is unloading, exit the thread.
         return 0;
      case 'r':
         thData->status = thread_device_read(thData->k_buffer, thData->k_length);
         complete(&threadData->from_thread_event);
         break;
      default:
         // Ignore any unexpected commands
         if (debugMode) {
            pr_err("thread_function() unexpected command %c\n", thData->command);
         }
         break;
      }

   }
   return 0;
}

/**
 * Print a notification when AlphaRNG device is initialized
 */
static void log_device_connect_message(void)
{
   pr_info("-------------------------------\n");
   pr_info("-- AlphaRNG device connected --\n");
   pr_info("-------------------------------\n");
}

/**
 * Configure statistical tests
 *
 */
static void configure_tests(void)
{
   numFailuresThreshold = 6;
   maxRctFailuresPerBlock = 0;
   maxAptFailuresPerBlock = 0;
   totalRctFailuresForCurrentDevice = 0;
   totalAptFailuresForCurrentDevice = 0;
   deviceStatusByte = 0;
   deviceTotalRequestsHandled = 0;
}

/**
 * A function to request new entropy bytes when running out of entropy in the local buffer
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int get_entropy_bytes(void)
{
   int status;
   if (curTrngOutIdx >= TRND_OUT_BUFFSIZE) {
      status = rcv_rnd_bytes();
   } else {
      status = SUCCESS;
   }
   return status;
}

/**
 * A function to fill the buffer with new entropy bytes
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int rcv_rnd_bytes(void)
{
   int retval;
   uint16_t byteCnt;

   if (!isEntropySrcRdy || isShutDown) {
      return -EPERM;
   }

   isUsbOpPending = true;
   retval = SUCCESS;

   if (ctrlData->isInitialized == false) {

	   // Clear the receive buffer before initializing the connection.
	   // This is to get rid of any previous response leftover data.
      clear_receive_buffer(USB_READ_TIMEOUT_SECS);

      // Initialize RCT and APT statistical tests
      rct_initialize();
      apt_initialize();
      if (disableStatisticalTests) {
         pr_info("AlphaRNG statistical tests: disabled\n");
      } else {
         pr_info("AlphaRNG statistical tests: enabled\n");
      }
      configure_tests();
      ctrlData->isInitialized = true;
   }

   if (retval == SUCCESS) {
      byteCnt = RND_IN_BUFFSIZE;

      ctrlData->bulk_out_buffer[0] = 120;

      retval = snd_rcv_usb_data(ctrlData->bulk_out_buffer, 1, buffRndIn, RND_IN_BUFFSIZE, USB_READ_TIMEOUT_SECS);
      if (retval == SUCCESS) {
         if (!disableStatisticalTests) {
            rct_restart();
            apt_restart();
            test_samples();
         }
         memcpy(buffTRndOut, buffRndIn, TRND_OUT_BUFFSIZE);
         curTrngOutIdx = TRND_OUT_BUFFSIZE - bytesPerSample;
         if (rct.statusByte != SUCCESS) {
            pr_err("%s: rcv_rnd_bytes(): Repetition Count Test failure\n", DRIVER_NAME);
            retval = -EPERM;
         } else if (apt.statusByte != SUCCESS) {
            pr_err("%s: rcv_rnd_bytes(): Adaptive Proportion Test failure\n", DRIVER_NAME);
            retval = -EPERM;
         }
      }
   }

   isUsbOpPending = false;
   return retval;
}

/**
 * Send command to the device and receive response
 *
 * @param snd -  a pointer to the command
 * @param sizeSnd - how many bytes in command
 * @param rcv - a pointer to the data receive buffer
 * @param sizeRcv - how many bytes expected to receive
 * @param opTimeoutSecs - device read time out value in seconds
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int snd_rcv_usb_data(char *snd, int sizeSnd, char *rcv, int sizeRcv, int opTimeoutSecs)
{
   int retry;
   int actualcCnt;
   int retval = SUCCESS;

   for (retry = 0; retry < USB_READ_MAX_RETRY_CNT; retry++) {
      if (isShutDown) {
         return -EPERM;
      }
      if (acmCtxt->device_locked == false) {
          retval = -EPERM;
      } else {
         // Send command to the ACM device
         actualcCnt = acm_write(acmCtxt->filed, snd, sizeSnd);
         if (actualcCnt > 0) {
            retval = SUCCESS;
         } else {
            retval = -EFAULT;
         }
      }
      if (retval == SUCCESS && actualcCnt == sizeSnd) {
         retval = chip_read_data(rcv, sizeRcv + 1, opTimeoutSecs);
         deviceTotalRequestsHandled++;
         if (retval == SUCCESS) {
            deviceStatusByte = (uint8_t)rcv[sizeRcv];
            if (rcv[sizeRcv] != 0) {
               retval = -EFAULT;
               clear_receive_buffer(opTimeoutSecs);
               if (debugMode) {
                  pr_err("AlphaRNG RNG: received device status code %d\n", rcv[sizeRcv]);
               }
            } else {
               break;
            }
         }
      } else {
         clear_receive_buffer(opTimeoutSecs);
         if (debugMode) {
            pr_err("snd_rcv_usb_data(): It was an error during data communication. Cleaning up the receiving queue and continue.\n");
         }
      }
   }
   if (retry >= USB_READ_MAX_RETRY_CNT) {
      retval = -ETIMEDOUT;
   }
   return retval;
}

/**
 * Function for clearing the receive buffer.
  *
 * @param opTimeoutSecs - device read time out value in seconds
 */
static void clear_receive_buffer(int opTimeoutSecs)
{
   while (chip_read_data(ctrlData->receiveClearBuff, sizeof(ctrlData->receiveClearBuff), opTimeoutSecs) == SUCCESS);
}

/**
 * A function to handle device read request
 *
 * @param buff - a pointer to the data receive buffer
 * @param length - how many bytes expected to receive
 * @param opTimeoutSecs - device read time out value in seconds

 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int chip_read_data(char *buff, int length, int opTimeoutSecs)
{
   long secsWaited;
   int transferred;
   ktime_t start, end;
   int cnt;
   int i;
   int retval;

   start = ktime_get();

   cnt = 0;
   do {
      if (isShutDown) {
         return -EPERM;
      }
      if (acmCtxt->device_locked == false) {
         retval = -EPERM;
      } else {
         // Retrieve data from the ACM device
         retval = acm_full_read(ctrlData->bulk_in_buffer, length, &transferred);
      }
      if (debugMode) {
         pr_info("chip_read_data() retval %d transferred %d, length %d\n", retval, transferred, length);
      }
      if (retval) {
         return retval;
      }

      if (transferred > USB_BUFFER_SIZE) {
         pr_err("%s: chip_read_data(): Received unexpected bytes when processing USB device request\n", DRIVER_NAME);
         return -EFAULT;
      }

      end = ktime_get();
      secsWaited = end - start;
      if (transferred > 0) {
         for (i = 0; i < transferred; i++) {
            buff[cnt++] = ctrlData->bulk_in_buffer[i];
         }
      }
   } while (cnt < length && secsWaited < opTimeoutSecs);

   if (cnt != length) {
      if (debugMode) {
         pr_info("chip_read_data(): timeout received, cnt %d\n", cnt);
      }
      return -ETIMEDOUT;
   }

   return SUCCESS;
}

/**
 * Initialize the character device
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 *
 */
static int init_char_dev(void)
{
   int error;
   int devices_to_destroy;
   dev_t dev;

   error = SUCCESS;
   dev = 0;
   devices_to_destroy = 0;

   error = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
   if (error < 0) {
      pr_err("%s: init_char_dev(): alloc_chrdev_region() call failed with error: %d\n", DRIVER_NAME, error);
      return error;
   }
   major = MAJOR(dev);

   dev_class = class_create(THIS_MODULE, DEVICE_NAME);
   if (IS_ERR(dev_class)) {
      error = PTR_ERR(dev_class);
      goto fail;
   }

   cdv = (struct cdev *) kzalloc(sizeof(struct cdev), GFP_KERNEL);
   if (cdv == NULL) {
      error = -ENOMEM;
      goto fail;
   }

   error = create_device();
   if (error) {
      goto fail;
   }

   return error;

fail:
   uninit_char_dev();
   return error;

}

/**
 * Create the device
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 */
static int create_device(void)
{
   int error;
   dev_t devno;
   struct device *device;

   error = SUCCESS;
   device = NULL;
   devno = MKDEV(major, minor);
   cdev_init(cdv, &fops);
   cdv->owner = THIS_MODULE;
   error = cdev_add(cdv, devno, 1);
   if (error) {
      pr_err("%s: create_device(): cdev_add() call failed with error: %d\n", DRIVER_NAME, error);
      return error;
   }
   device = device_create(dev_class, NULL, devno, NULL, DEVICE_NAME);
   if (IS_ERR(device)) {
      error = PTR_ERR(device);
      pr_err("%s: create_device(): device_create() failed with error: %d\n", DRIVER_NAME, error);
      return error;
   }

   return error;
}

/**
 * Create the proc
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 */
static int create_proc(void)
{
   // Create a directory under /proc
   proc_parent_dir = proc_mkdir(DEVICE_NAME, NULL);
   if (proc_parent_dir == NULL) {
      return -EPERM;
   }

   // Create the proc entry
   proc_create(PROC_NAME, 0444, proc_parent_dir, &proc_fops);
   return SUCCESS;
}

/**
 * Un-initialize the character device
 *
 */
static void uninit_char_dev(void)
{
   if (cdv) {
      device_destroy(dev_class, MKDEV(major, minor));
      cdev_del(cdv);
      kfree(cdv);
   }
   if (dev_class) {
      class_destroy(dev_class);
   }
   unregister_chrdev_region(MKDEV(major, 0), 1);
}

/**
 * Remove proc directory from /proc
 */
static void remove_proc(void)
{
   if (proc_parent_dir != NULL) {
      proc_remove(proc_parent_dir);
      proc_parent_dir = NULL;
   }
}

/**
 * A function to wait a little for any pending operations.
 * Used when unloading the module.
 *
 */
static void wait_for_pending_ops(void)
{
   int cnt;
   for (cnt = 0; cnt < 100 && (isDeviceOpPending == true || isProcOpPending == true || isUsbOpPending == true); cnt++) {
      msleep(500);
   }
}


/**
 * A function for testing a block of random bytes using 'repetition count'
 * and 'adaptive proportion' tests
 */
static void test_samples(void)
{
   uint8_t value;
   int i;

   for (i = 0; i < TRND_OUT_BUFFSIZE; i++) {
      value = buffRndIn[i];

      //
      // Run 'repetition count' test
      //
      if (!rct.isInitialized) {
         rct.isInitialized = true;
         rct.lastSample = value;
      } else {
         if (rct.lastSample == value) {
            rct.curRepetitions++;
            if (rct.curRepetitions >= rct.maxRepetitions) {
               rct.curRepetitions = 1;
               totalRctFailuresForCurrentDevice++;
               if (++rct.failureCount > numFailuresThreshold) {
                  if (rct.statusByte == 0) {
                     rct.statusByte = rct.signature;
                  }
               }

               if (rct.failureCount > maxRctFailuresPerBlock) {
                  // Record the maximum failures per block for reporting
                  maxRctFailuresPerBlock = rct.failureCount;
               }

               if (debugMode) {
                  if (rct.failureCount >= 1) {
                     pr_info("rct.failureCount: %d value: %d\n", rct.failureCount, value);
                  }
               }
            }

         } else {
            rct.lastSample = value;
            rct.curRepetitions = 1;
         }
      }

      //
      // Run 'adaptive proportion' test
      //
      if (!apt.isInitialized) {
         apt.isInitialized = true;
         apt.firstSample = value;
         apt.curRepetitions = 0;
         apt.curSamples = 0;
      } else {
         if (++apt.curSamples >= apt.windowSize) {
            apt.isInitialized = false;
            if (apt.curRepetitions > apt.cutoffValue) {
               // Check to see if we have reached the failure threshold
               totalAptFailuresForCurrentDevice++;
               if (++apt.cycleFailures > numFailuresThreshold) {
                  if (apt.statusByte == 0) {
                     apt.statusByte = apt.signature;
                  }
               }

               if (apt.cycleFailures > maxAptFailuresPerBlock) {
                  // Record the maximum failures per block for reporting
                  maxAptFailuresPerBlock = apt.cycleFailures;
               }

               if (debugMode) {
                  if (apt.cycleFailures >= 1) {
                     pr_info("apt.cycleFailures: %d value: %d\n", apt.cycleFailures, value);
                  }
               }

            }
         } else {
            if (apt.firstSample == value) {
               ++apt.curRepetitions;
            }
         }
      }
   }
}

/**
 * A function to initialize the repetition count test
 *
 */
static void rct_initialize(void)
{
   memset(&rct, 0x00, sizeof(rct));
   rct.statusByte = 0;
   rct.signature = 1;
   rct.maxRepetitions = 5;
   rct_restart();
}

/**
 * A function to restart the repetition count test
 *
 */
static void rct_restart(void)
{
   rct.isInitialized = false;
   rct.curRepetitions = 1;
   rct.failureCount = 0;
}

/**
 * A function to initialize the adaptive proportion test
 *
 */
static void apt_initialize(void)
{
   memset(&apt, 0x00, sizeof(apt));
   apt.statusByte = 0;
   apt.signature = 2;
   apt.windowSize = 64;
   apt.cutoffValue = 5;
   apt_restart();
}

/**
 * A function to restart the adaptive proportion test
 *
 */
static void apt_restart(void)
{
   apt.isInitialized = false;
   apt.cycleFailures = 0;
}

/*
 * A function for handling module loading event and for allocating resources.
 *
 * @return 0 - successful operation, otherwise the error code (a negative number)
 */
static int __init init_alrandom(void)
{
   int err;

   err = 0;
   buffRndIn = NULL;
   buffTRndOut = NULL;

   rct_initialize();
   apt_initialize();

   if (bytesPerSample <= 0 || bytesPerSample > TRND_OUT_BUFFSIZE) {
      pr_err("%s: init_alrandom(): Bytes per second parameter %d is not valid, it must be between 1 and 16000\n", DRIVER_NAME, bytesPerSample);
      return -EINVAL;
   }

   err = create_proc();
   if (err != SUCCESS) {
      pr_err("init_alrandom: could not create /proc/%s directory\n", DEVICE_NAME);
      return err;
   }

   err = init_char_dev();
   if (err != SUCCESS) {
      pr_err("%s: init_alrandom(): Could not initialize char device %s\n", DRIVER_NAME, DEVICE_NAME);
      remove_proc();
      return err;
   }

   // Initialize buffers and structures

   acmCtxt = kmalloc(sizeof(struct acm_context), GFP_KERNEL);
   if (acmCtxt == NULL) {
      pr_err("%s: init_alrandom(): Could not allocate bytes for the acm_context structure\n", DRIVER_NAME);
      err = -ENOMEM;
      goto acm_ctxt_mem_err;
   }

   buffRndIn = kmalloc(RND_IN_BUFFSIZE + 1, GFP_KERNEL);
   if (buffRndIn == NULL) {
      pr_err("%s: init_alrandom(): Could not allocate kernel bytes for the random input buffer\n", DRIVER_NAME);
      err = -ENOMEM;
      goto in_buff_mem_err;
   }

   buffTRndOut = kmalloc(TRND_OUT_BUFFSIZE, GFP_KERNEL);
   if (buffTRndOut == NULL) {
      pr_err("%s: init_alrandom(): Could not allocate kernel bytes for the random output buffer\n", DRIVER_NAME);
      err = -ENOMEM;
      goto out_buff_mem_err;
   }

   ctrlData = kmalloc(sizeof(struct ctrl_data), GFP_KERNEL);
   if (ctrlData == NULL) {
      pr_err("%s: init_alrandom(): Could not allocate kernel bytes for the control data\n", DRIVER_NAME);
      err = -ENOMEM;
      goto ctrl_mem_err;
   }

   threadData = kzalloc(sizeof(struct kthread_data), GFP_KERNEL);
   if (threadData == NULL) {
      pr_err("%s: init_alrandom(): Could not allocate kernel bytes for the kthread data\n", DRIVER_NAME);
      err = -ENOMEM;
      goto thread_mem_err;
   }

   // Give priority to ACM/CDC type when probing for AlphaRNG devices.
   acm_device_probe();

   threadData->drv_thread = kthread_run(thread_function, threadData, "AlphaRNG driver thread");
   if (threadData->drv_thread) {
      init_completion(&threadData->to_thread_event);
      init_completion(&threadData->from_thread_event);
   } else {
      pr_err("%s: init_alrandom(): Could not create a AlphaRNG driver kernel thread\n", DRIVER_NAME);
      err = -EPERM;
      goto thread_create_err;
   }

   mutex_init(&dataOpLock);

   pr_info("%s: Char device %s registered successfully, module version: %s\n", DRIVER_NAME, DEVICE_NAME, DRIVER_VERSION);

   return SUCCESS;

thread_create_err:
   kfree(threadData);
thread_mem_err:
   kfree(ctrlData);
ctrl_mem_err:
   kfree(buffTRndOut);
out_buff_mem_err:
   kfree(buffRndIn);
in_buff_mem_err:
   kfree(acmCtxt);
acm_ctxt_mem_err:
   remove_proc();
   uninit_char_dev();
   return err;
}

/**
 * Read bytes from ACM device.
 *
 * @param file - pointer to the file structure
 * @param data - pointer to where the bytes should be saved
 * @param size - amount of bytes to read
 *
 * @return if >= 0 then number of bytes retrieved, a negative number indicates an error
 *
 */
static int acm_read(struct file *file, unsigned char *data, int size)
{
   unsigned long long offset = 0;
   if (file == NULL) {
      return -1;
   }

   return kernel_read(file, data, size, &offset);
}

/**
 * Keep reading bytes from ACM device until all of the expected bytes are retrieved.
 * Stop reading when there are no more bytes to retrieve or when there is a failure
 * condition.
 *
 * @param data - pointer to where the bytes should be saved
 * @param size - total amount of bytes to read
 * @param bytesTransfered - pointer to an actual bytes transfered
 *
 * @return 0 for successful operation, a negative number indicates an error.
 *
 */
static int acm_full_read(unsigned char *data, int size, int *bytesTransfered)
{
   int bytesReceived = 0;
   int totalBytesReceived = 0;

   while (totalBytesReceived < size) {
      bytesReceived = acm_read(acmCtxt->filed, data + totalBytesReceived, size - totalBytesReceived);
      if (bytesReceived < 0) {
         pr_err("%s: acm_full_read(): Could not receive data from ACM device\n", DRIVER_NAME);
         return -EPERM;
      }
      if (bytesReceived == 0) {
         return -EPERM;
      }
      totalBytesReceived += bytesReceived;
   }
   if (totalBytesReceived != size) {
      return -EPERM;
   }
   *bytesTransfered = totalBytesReceived;
   return SUCCESS;
}

/**
 * Write bytes to ACM device.
 *
 * @param size - amount of bytes to write
 * @param data - pointer to bytes to write
 * @param file - pointer to the file structure
 *
 * @return if >= 0 then number of bytes written, a negative number indicates an error
 *
 */
static int acm_write(struct file *file, const unsigned char *data, int size)
{
   unsigned long long offset = 0;

   if (file == NULL) {
      return -1;
   }

   return kernel_write(file, data, size, &offset);
}

/**
 * Close ACM device
 *
 * @param file - pointer to the file structure
 */
static void acm_close(struct file *file)
{
   if (file != NULL) {
      filp_close(file, NULL);
   }
}

/**
 * Open an ACM device
 *
 * @param flags - for example: O_RDWR
 * @param path - path to the device
 *
 * @return pointer to a file structure or NULL when error
 */
static struct file *acm_open(const char *path, int flags)
{
   struct file *filp = NULL;
   filp = filp_open(path, flags, 0);
   if (IS_ERR(filp)) {
      return NULL;
   }
   return filp;
}

/**
 * A function used for searching for AlphaRNG ACM devices
 */
static int acm_iterate_dir_callback(struct dir_context *ctx, const char *name, int nameLength, loff_t offset, u64 ino,
      unsigned int dType)
{
   struct acm_callback_context *buf = container_of(ctx, struct acm_callback_context, ctx);
   return buf->filler(buf->context, name, nameLength, offset, ino, dType);
}

/**
 * A function used when searching for AlphaRNG ACM devices
 *
 * @return 0 for successful operation
 */
static int acm_readdir(const char* path, acm_readdir_t filler, void* context)
{
   int res;
   struct acm_callback_context buf = { .ctx.actor = (filldir_t) acm_iterate_dir_callback, .context = context, .filler =
         filler };

   struct file* dir = filp_open(path, O_DIRECTORY, S_IRWXU | S_IRWXG | S_IRWXO);
   if (!IS_ERR(dir)) {

      res = iterate_dir(dir, &buf.ctx);
      filp_close(dir, NULL);
   } else {
      res = (int) PTR_ERR(dir);
   }
   return res;
}

/**
 * A function used when searching for AlphaRNG ACM devices
 */
static int acm_filldir_callback(void* data, const char *name, int nameLength, loff_t offset, u64 ino, unsigned int dType)
{
   if (nameLength >= ACM_DEV_NAME_BY_ID_LENGTH_LIMIT - 1) {
      pr_err("%s: acm_filldir_callback(): ACM device name too long\n", DRIVER_NAME);
      return 0;
   }
   if (acmCtxt->devices_found >= MAX_ACM_DEVICES_TO_PROBE) {
      pr_err("%s: acm_filldir_callback(): Exceeding max number of ACM devices\n", DRIVER_NAME);
      return 0;
   }
   memcpy(acmCtxt->dev_name_by_id[acmCtxt->devices_found], name, nameLength);
   acmCtxt->dev_name_by_id[acmCtxt->devices_found][nameLength] = '\0';
   if (strstr(acmCtxt->dev_name_by_id[acmCtxt->devices_found], "TectroLabs_Alpha_RNG") != NULL) {
      acmCtxt->devices_found++;
   }
   return 0;
}

/**
 * Search for a suitable AlphaRNG ACM device
 *
 * @return true if at least one device is found
 */
static bool acm_search_for_device(void)
{
   acm_readdir(dev_serial_by_id_path, acm_filldir_callback, (void*) 1);
   if (!acmCtxt->devices_found) {
      pr_info("%s: No AlphaRNG CDC/ACM device found\n", DRIVER_NAME);
      return false;
   }
   return true;
}

/**
 * Initialize control data for probe operations.
 */
static void probe_init(void)
{
   ctrlData->isInitialized = false;

   // Reset TRNG buffer index
   curTrngOutIdx = TRND_OUT_BUFFSIZE;
}

/**
 * Probe for AlphaRNG ACM devices.
 *
 * @return true if device is found and ready for usage
 */
static bool acm_device_probe(void)
{
   int op_status = 0;
   int i;

   if (acmCtxt == NULL) {
      pr_err("%s: acm_device_probe(): BUG: acmCtxt not initialized\n", DRIVER_NAME);
      return false;
   }

   if (isEntropySrcRdy) {
      pr_err("%s: acm_device_probe(): BUG: probing for ACM devices with an entropy source active\n", DRIVER_NAME);
      return false;
   }

   // Clear the ACM context
   memset(acmCtxt, 0, sizeof(struct acm_context));
   probe_init();

   if (!acm_search_for_device()) {
      return false;
   }

   for (i = 0; i < acmCtxt->devices_found; ++i) {

      strcpy(acmCtxt->dev_name, dev_serial_by_id_path);
      if (debugMode) {
         pr_info("found AlphaRNG device: %s\n", acmCtxt->dev_name_by_id[i]);
      }
      strcat(acmCtxt->dev_name, "/");
      strcat(acmCtxt->dev_name, acmCtxt->dev_name_by_id[i]);

      op_status = kern_path(acmCtxt->dev_name, LOOKUP_FOLLOW, &acmCtxt->path);
      if (op_status) {
         if (debugMode) {
            pr_err("%s: acm_device_probe(): kern_path() failed for %s\n", DRIVER_NAME, acmCtxt->dev_name);
         }
         continue;
      }
      acmCtxt->inode = acmCtxt->path.dentry->d_inode;
      acmCtxt->devt = acmCtxt->inode->i_rdev;

      if (!acm_set_tty_termios_flags()) {
         continue;
      }

      if (!acm_open_device()) {
         continue;
      }

      if (!acm_lock_device()) {
         acm_clean_up();
         continue;
      }

      if (debugMode) {
         pr_info("acm_context->devices_found: %d\n", acmCtxt->devices_found);
         pr_info("acm_context->device_open: %d\n", acmCtxt->device_open);
         pr_info("acm_context->device_locked: %d\n", acmCtxt->device_locked);
      }
      isEntropySrcRdy = true;

      log_device_connect_message();

      return true;
   }

   return false;
}

/**
 * Open an ACM device
 *
 * @return true for successful operation
 */
static bool acm_open_device(void)
{
   acmCtxt->filed = acm_open(acmCtxt->dev_name, O_RDWR | O_NOCTTY | O_SYNC);
   if (acmCtxt->filed == NULL) {
      pr_info("%s: acm_open_device(): Could not open tty device: %s", DRIVER_NAME, acmCtxt->dev_name);
      return false;
   }
   acmCtxt->device_open = true;
   return true;
}

/**
 * Lock an ACM device
 *
 * @return true for successful operation
 */
static bool acm_lock_device(void)
{
   int op_status;
   locks_init_lock(&acmCtxt->fl);
   acmCtxt->fl.fl_flags = FL_FLOCK | FL_EXISTS;
   acmCtxt->fl.fl_type = LOCK_READ | LOCK_WRITE;
   op_status = locks_lock_inode_wait(acmCtxt->inode, &acmCtxt->fl);
   if (op_status != 0) {
      pr_info("%s: acm_lock_device(): Could not lock device %s\n", DRIVER_NAME, acmCtxt->dev_name);
      return false;
   }
   acmCtxt->device_locked = true;
   return true;
}

/**
 * Set termios flags associated with the ACM tty device
 *
 * @return true for successful operation
 */
static bool acm_set_tty_termios_flags(void)
{
   bool successStatus = true;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,11,22)
   acmCtxt->tty = tty_kopen(acmCtxt->devt);
#else
   acmCtxt->tty = tty_kopen_exclusive(acmCtxt->devt);
#endif

   if (IS_ERR(acmCtxt->tty)) {
      pr_info("%s: tty_kopen() failed for %s\n", DRIVER_NAME, acmCtxt->dev_name);
      return false;
   }

   acmCtxt->opts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
   acmCtxt->opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
   acmCtxt->opts.c_oflag &= ~(ONLCR | OCRNL);

   // Set time out to 100 milliseconds for read serial device operations
   acmCtxt->opts.c_cc[VTIME] = 1;
   acmCtxt->opts.c_cc[VMIN] = 0;

   if (tty_set_termios(acmCtxt->tty, &acmCtxt->opts) != 0) {
      pr_info("%s: acm_set_tty_termios_flags(): tty_set_termios() failed for %s\n", DRIVER_NAME, acmCtxt->dev_name);
      successStatus = false;
      goto close_free_tty;
   }

close_free_tty:
   tty_kclose(acmCtxt->tty);
   tty_kref_put(acmCtxt->tty);
   return successStatus;
}

/**
 * Clean up ACM data
 */
static void acm_clean_up(void)
{
   int op_status;

   if (acmCtxt == NULL) {
      pr_err("%s: acm_clean_up(): BUG: acmCtxt not initialized\n", DRIVER_NAME);
      return;
   }

   if (acmCtxt->device_locked) {
      acmCtxt->fl.fl_type = F_UNLCK;
      op_status = locks_lock_inode_wait(acmCtxt->inode, &acmCtxt->fl);
      if (op_status != 0) {
         pr_err("%s: acm_clean_up(): Could not unlock %s\n", DRIVER_NAME, acmCtxt->dev_name);
      }
      acmCtxt->device_locked = false;
      if (debugMode) {
         pr_info("acm_clean_up(): locks_lock_inode_wait() returned: %d\n", op_status);
      }
   }

   if (acmCtxt->device_open) {
      acm_close(acmCtxt->filed);
      acmCtxt->device_open = false;
   }

   isEntropySrcRdy = false;
}

/*
 * A function to handle module unloading event
 */
static void __exit exit_alrandom(void)
{
   isEntropySrcRdy = false;
   isShutDown = true;

   if (!completion_done(&threadData->to_thread_event)) {
      threadData->command = 'e';
      complete(&threadData->to_thread_event);
   }

   msleep(1000);
   wait_for_pending_ops();
   acm_clean_up();
   remove_proc();
   uninit_char_dev();
   kfree(buffRndIn);
   kfree(buffTRndOut);
   kfree(acmCtxt);
   kfree(ctrlData);
   kfree(threadData);
   mutex_destroy(&dataOpLock);
   pr_info("%s: exit_alrandom(): Char device %s unregistered successfully\n", DRIVER_NAME, DEVICE_NAME);
}

module_init( init_alrandom);
module_exit( exit_alrandom);
