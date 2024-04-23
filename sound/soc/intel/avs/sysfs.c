// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright(c) 2021-2024 Intel Corporation. All rights reserved.
//
// Authors: Cezary Rojewski <cezary.rojewski@intel.com>
//          Amadeusz Slawinski <amadeuszx.slawinski@linux.intel.com>
//

#include <linux/sysfs.h>
#include "avs.h"
#include "messages.h"

static ssize_t fw_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct avs_dev *adev = to_avs_dev(dev);
	struct avs_fw_version *fw_version = &adev->fw_cfg.fw_version;

	return sysfs_emit(buf, "%d.%d.%d.%d\n", fw_version->major, fw_version->minor,
			  fw_version->hotfix, fw_version->build);
}
static DEVICE_ATTR_RO(fw_version);

static ssize_t sched_cfg_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct avs_dev *adev = to_avs_dev(dev);
	int ret;

	mutex_lock(&adev->path_mutex);

	if (!list_empty(&adev->path_list)) {
		mutex_unlock(&adev->path_mutex);
		return -EBUSY;
	}

	ret = avs_parse_sched_cfg(adev, buf, count);
	if (ret < 0) {
		mutex_unlock(&adev->path_mutex);
		return ret;
	}

	pm_runtime_get_sync(dev);
	ret = avs_ipc_set_fw_config(adev, 1, AVS_FW_CFG_SCHEDULER_CONFIG,
				    sizeof(*adev->sched_cfg), adev->sched_cfg);
	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);

	mutex_unlock(&adev->path_mutex);

	if (!ret)
		ret = count;
	return ret;
}
static DEVICE_ATTR_WO(sched_cfg);

static struct attribute *avs_fw_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_sched_cfg.attr,
	NULL
};

static const struct attribute_group avs_attr_group = {
	.name = "avs",
	.attrs = avs_fw_attrs,
};

const struct attribute_group *avs_attr_groups[] = {
	&avs_attr_group,
	NULL
};
