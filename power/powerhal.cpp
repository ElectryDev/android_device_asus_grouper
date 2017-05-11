/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "PowerHAL"

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "powerhal_utils.h"
#include "powerhal.h"

#define INTERACTIVE_PATH    "/sys/devices/system/cpu/cpufreq/interactive/"
#define CPUQUIET_PATH       "/sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/"
#define CPUFREQ_CPU0_PATH   "/sys/devices/system/cpu/cpu0/cpufreq/"
#define NVAVP_PATH          "/sys/devices/platform/host1x/nvavp/"

#define INPUT_PATH          "/sys/class/input/"

static struct input_dev_map input_devs[] = {
    {-1, "elan-touchscreen\n"}
};

#define MAX_FREQ_DEFAULT 1300000

#define BUF_SIZE 80
#define MAX_CHARS 32

static struct powerhal_info *pInfo;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * grouper frequency table
 *
 * 51
 * 102
 * 204
 * 340 MHz
 * 475 MHz
 * 640 MHz
 * 760 MHz
 * 860 MHz
 * 1000 MHz
 * 1100 MHz
 * 1200 MHz
 * 1300 MHz (single core/OC)
 * 1400 MHz (OC)
 * 1500 MHz (OC)
 */

static void find_input_device_ids()
{
    int i = 0;
    int status;
    int count = 0;
    char path[80];
    char name[MAX_CHARS];

    while (1)
    {
        snprintf(path, sizeof(path), INPUT_PATH "input%d/name", i);
        if (access(path, F_OK) < 0)
            break;
        else {
            memset(name, 0, MAX_CHARS);
            sysfs_read(path, name, MAX_CHARS);
            for (int j = 0; j < pInfo->input_cnt; j++) {
                status = (-1 == pInfo->input_devs[j].dev_id)
                    && (0 == strncmp(name,
                    pInfo->input_devs[j].dev_name, MAX_CHARS));
                if (status) {
                    ++count;
                    pInfo->input_devs[j].dev_id = i;
                    ALOGI("find_input_device_ids: %d %s",
                        pInfo->input_devs[j].dev_id,
                        pInfo->input_devs[j].dev_name);
                }
            }
            ++i;
        }

        if (count == pInfo->input_cnt)
            break;
    }
}

static int is_profile_valid(int profile)
{
    return profile >= 0 && profile < PROFILE_MAX;
}

static bool is_interactive(void)
{
    struct stat s;
    int err = stat(INTERACTIVE_PATH, &s);
    if (err != 0) return false;
    if (S_ISDIR(s.st_mode)) return true;
    return false;
}

static int open_w(char *path, int *fd)
{
    if (*fd < 0) {
        *fd = open(path, O_WRONLY);
    }

    return *fd;
}

static void boostpulse()
{
    char buf[BUF_SIZE];
    int len;

    pthread_mutex_lock(&lock);
    if (open_w(INTERACTIVE_PATH "boostpulse",
            &pInfo->boostpulse_fd) >= 0) {
        snprintf(buf, sizeof(buf), "%d", 1);

        len = write(pInfo->boostpulse_fd, &buf, sizeof(buf));
        if (len < 0) {
            strerror_r(errno, buf, sizeof(buf));
            ALOGE("Error writing to boostpulse: %s\n", buf);

            close(pInfo->boostpulse_fd);
            pInfo->boostpulse_fd = -1;
        }
    }
    pthread_mutex_unlock(&lock);
}

void power_init(struct power_module *module __unused)
{
    char buf[BUF_SIZE];

    if (!pInfo)
        pInfo = (powerhal_info*)calloc(1, sizeof(powerhal_info));

    pInfo->input_devs = input_devs;
    pInfo->input_cnt = sizeof(input_devs)/sizeof(struct input_dev_map);

    // Match input device ids to names
    find_input_device_ids();

    // Read maximum frequency
    sysfs_read(CPUFREQ_CPU0_PATH "scaling_max_frequency", buf, BUF_SIZE);
    pInfo->max_frequency = atoi(buf);
    pInfo->is_overclocked = pInfo->max_frequency > MAX_FREQ_DEFAULT ? true : false;

    // Store LP cluster max frequency
    sysfs_read(CPUQUIET_PATH "idle_top_freq", buf, BUF_SIZE);
    pInfo->lp_max_frequency = atoi(buf);

    pInfo->current_power_profile = PROFILE_BALANCED;
    pInfo->boostpulse_fd = -1;
}

void power_set_interactive(struct power_module *module __unused, int on)
{
    int i;
    int dev_id;
    char path[80];

    if (!pInfo)
        return;

    ALOGI("%s: setting interactive: %d", __func__, on);

    pthread_mutex_lock(&lock);

    sysfs_write(CPUQUIET_PATH "no_lp", on ? "1" : "0");
    ALOGI("Setting low power cluster: %sabled", on ? "dis" : "en");

    sysfs_write(NVAVP_PATH "boost_sclk", on ? "1" : "0");
    ALOGI("Setting boost_sclk: %sabled", on ? "en" : "dis");

    for (i = 0; i < pInfo->input_cnt; i++) {
        if (pInfo->input_devs[i].dev_id == -1)
            continue;
        else
            dev_id = pInfo->input_devs[i].dev_id;

        snprintf(path, sizeof(path), INPUT_PATH "input%d/enabled", dev_id);
        if (!access(path, W_OK)) {
            ALOGI("%sabling input device: %d", on ? "En" : "Dis", dev_id);
            sysfs_write(path, on ? "1" : "0");
        }
    }

    if (!is_interactive())
        return;

    if (on) {
        /* interactive */
        sysfs_write_int(INTERACTIVE_PATH "hispeed_freq",
                        pInfo->is_overclocked ?
                        profiles[pInfo->current_power_profile].hispeed_freq_oc :
                        profiles[pInfo->current_power_profile].hispeed_freq);
        sysfs_write_int(INTERACTIVE_PATH "go_hispeed_load",
                        profiles[pInfo->current_power_profile].go_hispeed_load);
        sysfs_write(INTERACTIVE_PATH "target_loads",
                        profiles[pInfo->current_power_profile].target_loads);
    } else {
        /* interactive */
        sysfs_write_int(INTERACTIVE_PATH "hispeed_freq",
                        profiles[pInfo->current_power_profile].hispeed_freq_off);
        sysfs_write_int(INTERACTIVE_PATH "go_hispeed_load",
                        profiles[pInfo->current_power_profile].go_hispeed_load_off);
        sysfs_write(INTERACTIVE_PATH "target_loads",
                        profiles[pInfo->current_power_profile].target_loads_off);
    }

    pthread_mutex_unlock(&lock);
}

static void set_power_profile(int profile)
{
    if (!is_profile_valid(profile)) {
        ALOGE("%s: unknown profile: %d", __func__, profile);
        return;
    }

    ALOGI("%s: setting profile: %d", __func__, profile);

    pthread_mutex_lock(&lock);

    /* interactive */
    sysfs_write_int(INTERACTIVE_PATH "boostpulse_duration",
                    profiles[profile].boostpulse_duration);
    sysfs_write_int(INTERACTIVE_PATH "go_hispeed_load",
                    profiles[profile].go_hispeed_load);
    sysfs_write_int(INTERACTIVE_PATH "hispeed_freq",
                    pInfo->is_overclocked ?
                    profiles[profile].hispeed_freq_oc :
                    profiles[profile].hispeed_freq);
    sysfs_write_int(INTERACTIVE_PATH "io_is_busy",
                    profiles[profile].io_is_busy);
    sysfs_write_int(INTERACTIVE_PATH "min_sample_time",
                    profiles[profile].min_sample_time);
    sysfs_write(INTERACTIVE_PATH "target_loads",
                    profiles[profile].target_loads);

    pInfo->current_power_profile = profile;
    pthread_mutex_unlock(&lock);
}

void power_hint(struct power_module *module __unused,
        power_hint_t hint, void *data __unused)
{
    if (!pInfo)
        return;

    if (!is_interactive())
        return;

    switch (hint) {
    case POWER_HINT_LOW_POWER:
        break;
    case POWER_HINT_INTERACTION:
        break;
    case POWER_HINT_LAUNCH:
        ALOGV("POWER_HINT_LAUNCH");
        if (profiles[pInfo->current_power_profile].boostpulse_duration)
            break;
        boostpulse();
        break;
    case POWER_HINT_SET_PROFILE:
        ALOGV("POWER_HINT_SET_PROFILE: %d", (*(int32_t *)data));
        set_power_profile(*(int32_t *)data);
        break;
    default:
        ALOGE("Unknown power hint: 0x%x", hint);
        break;
    }
}

static void power_set_feature(struct power_module *module __unused,
        feature_t feature, int state __unused)
{
    switch (feature) {
    case POWER_FEATURE_DOUBLE_TAP_TO_WAKE:
        ALOGW("Double tap to wake is not supported\n");
        break;
    default:
        ALOGW("Error setting the feature, it doesn't exist %d\n", feature);
        break;
    }
}

static int power_get_feature(struct power_module *module __unused,
        feature_t feature)
{
    if (feature == POWER_FEATURE_SUPPORTED_PROFILES) {
        return PROFILE_MAX;
    }

    return -1;
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_3,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "Grouper Power HAL",
        .author = "The LineageOS Project",
        .methods = &power_module_methods,
    },

    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
    .setFeature = power_set_feature,
    .getFeature = power_get_feature,
};
