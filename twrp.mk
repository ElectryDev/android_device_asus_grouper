RECOVERY_VARIANT := twrp
RECOVERY_SDCARD_ON_DATA := true

PRODUCT_COPY_FILES += \
    device/asus/grouper/recovery/root/etc/twrp.fstab:recovery/root/etc/twrp.fstab

BOARD_HAS_NO_REAL_SDCARD := true
TW_THEME := portrait_hdpi
TW_BRIGHTNESS_PATH := \
    /sys/devices/platform/pwm-backlight/backlight/pwm-backlight/brightness
TW_NO_USB_STORAGE := false
TW_INCLUDE_L_CRYPTO := true
