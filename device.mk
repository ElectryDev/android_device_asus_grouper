#
# Copyright (C) 2010 The Android Open Source Project
#           (C) 2017 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# The actual meat of the device-specific product definition
$(call inherit-product, device/asus/grouper/device-common.mk)

DEVICE_PACKAGE_OVERLAYS += \
    device/asus/grouper/overlay

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/rootdir/fstab.grouper:root/fstab.grouper \
    $(LOCAL_PATH)/rootdir/init.grouper.rc:root/init.grouper.rc \
    $(LOCAL_PATH)/rootdir/init.grouper.power.rc:root/init.grouper.power.rc \
    $(LOCAL_PATH)/rootdir/init.grouper.sensors.rc:root/init.grouper.sensors.rc

PRODUCT_PROPERTY_OVERRIDES += \
    ro.carrier=wifi-only
