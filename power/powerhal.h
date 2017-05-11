/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef _POWER_HAL_H_
#define _POWER_HAL_H_

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "powerhal_utils.h"

#include <semaphore.h>
#include <stdlib.h>

struct input_dev_map {
    int dev_id;
    const char* dev_name;
};

struct powerhal_info {
    /* Maximum CPU frequency */
    int max_frequency;
    bool is_overclocked;

    /* Maximum LP CPU frequency */
    int lp_max_frequency;

    /* Number of devices requesting Power HAL service */
    int input_cnt;

    /* Holds input devices */
    struct input_dev_map* input_devs;

    int boostpulse_fd;

    int current_power_profile;
};

enum {
    PROFILE_POWER_SAVE = 0,
    PROFILE_BALANCED,
    PROFILE_HIGH_PERFORMANCE,
    PROFILE_BIAS_POWER_SAVE,
    PROFILE_BIAS_PERFORMANCE,
    PROFILE_MAX
};

typedef struct governor_settings {
    /* interactive */
    int boostpulse_duration;
    int go_hispeed_load;
    int go_hispeed_load_off;
    int hispeed_freq;
    int hispeed_freq_oc;
    int hispeed_freq_off;
    int io_is_busy;
    int min_sample_time;
    char *target_loads;
    char *target_loads_off;
    /* cpu */
    int max_cpu_freq;
    int min_cpu_freq;
    int min_cpu_freq_off;
    int max_cpu_online;
    int min_cpu_online;
} power_profile;

static power_profile profiles[PROFILE_MAX] = {
    [PROFILE_POWER_SAVE] = {
        /* interactive */
        .boostpulse_duration =    0,
        .go_hispeed_load =        95,
        .go_hispeed_load_off =    95,
        .hispeed_freq =           760000,
        .hispeed_freq_oc =        760000,
        .hispeed_freq_off =       760000,
        .io_is_busy =             0,
        .min_sample_time =        60000,
        .target_loads =           "95",
        .target_loads_off =       "95",
        /* cpu */
        .max_cpu_freq =           1100000,
        .min_cpu_freq =           51000,
        .min_cpu_freq_off =       51000,
        .max_cpu_online =         2,
        .min_cpu_online =         1,
    },
    [PROFILE_BALANCED] = {
        /* interactive */
        .boostpulse_duration =    200000,
        .go_hispeed_load =        95,
        .go_hispeed_load_off =    95,
        .hispeed_freq =           1000000,
        .hispeed_freq_oc =        1200000,
        .hispeed_freq_off =       760000,
        .io_is_busy =             1,
        .min_sample_time =        40000,
        .target_loads =           "70 1200000:80 1300000:85 1400000:90",
        .target_loads_off =       "90 1200000:99",
        /* cpu */
        .max_cpu_freq =           -1,
        .min_cpu_freq =           51000,
        .min_cpu_freq_off =       51000,
        .max_cpu_online =         4,
        .min_cpu_online =         1,
    },
    [PROFILE_HIGH_PERFORMANCE] = {
        /* interactive */
        .boostpulse_duration =    1000000,
        .go_hispeed_load =        75,
        .go_hispeed_load_off =    75,
        .hispeed_freq =           1200000,
        .hispeed_freq_oc =        1400000,
        .hispeed_freq_off =       1000000,
        .io_is_busy =             1,
        .min_sample_time =        40000,
        .target_loads =           "70",
        .target_loads_off =       "70",
        /* cpu */
        .max_cpu_freq =           -1,
        .min_cpu_freq =           1000000,
        .min_cpu_freq_off =       475000,
        .max_cpu_online =         4,
        .min_cpu_online =         4,
    },
    [PROFILE_BIAS_POWER_SAVE] = {
        /* interactive */
        .boostpulse_duration =    100000,
        .go_hispeed_load =        95,
        .go_hispeed_load_off =    95,
        .hispeed_freq =           860000,
        .hispeed_freq_oc =        1100000,
        .hispeed_freq_off =       640000,
        .io_is_busy =             1,
        .min_sample_time =        40000,
        .target_loads =           "70 1200000:85 1300000:90 1400000:95",
        .target_loads_off =       "95",
        /* cpu */
        .max_cpu_freq =           -1,
        .min_cpu_freq =           51000,
        .min_cpu_freq_off =       51000,
        .max_cpu_online =         4,
        .min_cpu_online =         1,
    },
    [PROFILE_BIAS_PERFORMANCE] = {
        /* interactive */
        .boostpulse_duration =    500000,
        .go_hispeed_load =        90,
        .go_hispeed_load_off =    90,
        .hispeed_freq =           1100000,
        .hispeed_freq_oc =        1300000,
        .hispeed_freq_off =       860000,
        .io_is_busy =             1,
        .min_sample_time =        40000,
        .target_loads =           "70 1200000:75 1300000:80 1400000:90",
        .target_loads_off =       "90",
        /* cpu */
        .max_cpu_freq =           -1,
        .min_cpu_freq =           51000,
        .min_cpu_freq_off =       51000,
        .max_cpu_online =         4,
        .min_cpu_online =         2,
    },
};

#endif

