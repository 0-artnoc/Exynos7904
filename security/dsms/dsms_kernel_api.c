/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <asm/uaccess.h>

#include <linux/dsms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "dsms_access_control.h"
#include "dsms_debug.h"
#include "dsms_init.h"

#define VALUE_STRLEN	(22)

// Command: <<DSMS_BINARY>> <<feature_code>> <<detail>> <<value>>
#define DSMS_BINARY "/system/bin/dsms"
static const char *dsms_command[] = {
	DSMS_BINARY,
	NULL,
	NULL,
	NULL,
	NULL
};
#define FEATURE_INDEX (1)
#define EXTRA_INDEX (2)
#define VALUE_INDEX (3)

#define MESSAGE_COUNT_LIMIT (50)

static const char *dsms_environ[] = {
	"HOME=/",
	"PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin",
	"ANDROID_DATA=/data",
	NULL
};

static atomic_t message_counter = ATOMIC_INIT(0);

static char *dsms_alloc_user_string(const char *string)
{
	size_t size;
	char *string_cpy;
	if (string == NULL || *string == 0)
		return "";

	size = strnlen(string, PAGE_SIZE - 1) + 1;
	string_cpy = (char *) kmalloc(size * sizeof(string[0]),
				      GFP_USER);
	if (string_cpy) {
		memcpy(string_cpy, string, size);
		string_cpy[size - 1] = '\0';
	}

	return string_cpy;
}

static char *dsms_alloc_user_value(int64_t value)
{
	char *string = (char *) kmalloc(VALUE_STRLEN, GFP_USER);
	if (string) {
		snprintf(string, VALUE_STRLEN, "%lld", value);
		string[VALUE_STRLEN-1] = 0;
	}
	return string;
}

static void dsms_free_user_string(const char *string)
{
	if (string == NULL || *string == 0)
		return;
	kfree(string);
}

static void dsms_message_cleanup(struct subprocess_info *info)
{
	if (info && info->argv) {
		dsms_free_user_string(info->argv[FEATURE_INDEX]);
		dsms_free_user_string(info->argv[EXTRA_INDEX]);
		dsms_free_user_string(info->argv[VALUE_INDEX]);
		kfree(info->argv);
	}
	atomic_dec(&message_counter);
}

static int check_recovery_mode(void)
{
	static int recovery = -1;
	struct file *fp;

	if (recovery < 0) {
		fp = filp_open("/sbin/recovery", O_RDONLY, 0);
		if (IS_ERR(fp)) {
			printk(KERN_ALERT DSMS_TAG "Normal mode.\n");
			recovery = 0;
		} else {
			filp_close(fp, NULL);
			printk(KERN_ALERT DSMS_TAG "Recovery mode, DSMS is disabled.\n");
			recovery = 1;
		}
	}
	return (recovery > 0);
}

static inline int dsms_send_allowed_message(const char *feature_code,
		const char *detail,
		int64_t value)
{
	char **argv;
	struct subprocess_info *info;
	int ret;

	// limit number of message to prevent message's bursts
	if (atomic_add_unless(&message_counter, 1, MESSAGE_COUNT_LIMIT) == 0) {
		printk(DSMS_TAG "Message counter has reached its limit.");
		ret = -EBUSY;
		goto limit_error;
	}
	// allocate argv, envp, necessary data
	argv = (char**) kmalloc(sizeof(dsms_command), GFP_USER);
	if (!argv) {
		printk(DSMS_TAG "Failed memory allocation for argv.");
		ret = -ENOMEM;
		goto no_mem_error;
	}

	memcpy(argv, dsms_command, sizeof(dsms_command));

	argv[FEATURE_INDEX] = dsms_alloc_user_string(feature_code);
	argv[EXTRA_INDEX] = dsms_alloc_user_string(detail);
	argv[VALUE_INDEX] = dsms_alloc_user_value(value);
	if (!argv[FEATURE_INDEX] || !argv[EXTRA_INDEX] ||
	    !argv[VALUE_INDEX]) {
		printk(DSMS_TAG "Failed memory allocation for user string.");
		ret = -ENOMEM;
		goto no_mem_error;
	}

	// call_usermodehelper with wait_proc and callback function to cleanup data after execution
	info = call_usermodehelper_setup(DSMS_BINARY, argv,
					 (char**) dsms_environ,
					 GFP_ATOMIC, NULL,
					 &dsms_message_cleanup, NULL);
	if (!info) {
		printk(DSMS_TAG "Failed memory allocation for"
		       "call_usermodehelper_setup.");
		ret = -ENOMEM;
		goto no_mem_error;
	}

	return call_usermodehelper_exec(info, UMH_NO_WAIT);

no_mem_error:
	if (argv) {
		dsms_free_user_string(argv[FEATURE_INDEX]);
		dsms_free_user_string(argv[EXTRA_INDEX]);
		dsms_free_user_string(argv[VALUE_INDEX]);
		kfree(argv);
	}
	atomic_dec(&message_counter);

limit_error:
	return ret;
}

int noinline dsms_send_message(const char *feature_code,
		const char *detail,
		int64_t value)
{
	void *address;
	int ret;

#ifdef DSMS_KERNEL_ENG /* skip dsms at eng binary */
	printk(KERN_ALERT DSMS_TAG "It's ENG binary, skip...");
	return DSMS_SUCCESS;
#endif /* DSMS_KERNEL_ENG */

	// check recovery mode on, dsms doesn't work in recovery mode
	if (check_recovery_mode()) {
		printk(KERN_ALERT DSMS_TAG "Recovery mode, DSMS is disabled.");
		return DSMS_SUCCESS;
	}

#ifdef DSMS_DEBUG_TRACE_DSMS_CALLS
	dsms_debug_message(feature_code, detail, value);
#endif //DSMS_DEBUG_TRACE_DSMS_CALLS
	
	if (!dsms_is_initialized()) {
#ifdef DSMS_DEBUG_ENABLE
		printk(DSMS_DEBUG_TAG "DSMS not initialized yet.");
#endif //DSMS_DEBUG_ENABLE
		ret = -EACCES;
		goto exit_send;
	}

	address = __builtin_return_address(CALLER_FRAME);
	ret = dsms_verify_access(address);
	if (ret != DSMS_SUCCESS)
		goto exit_send;
	
	ret = dsms_send_allowed_message(feature_code, detail, value);
	
exit_send:
	return ret;
}
