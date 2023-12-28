@echo off

if "%PYTHONNOUSERSITE%" == "" (
    echo Run from ESP-IDF console!
	pause
	exit
)

@RD /S /Q build-v0-8m
@RD /S /Q build-v1-8m
@RD /S /Q build-v1-16m
@RD /S /Q build-v1-32m

idf.py -B build-v0-8m -DSDKCONFIG=build-v0-8m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v0_8m" build
REM nie dziala - esptool.py --chip ESP32S3 merge_bin -o build-v0-8m\merged-firmware.bin --flash_mode dio --flash_size 8MB 0x0 build-v0-8m\bootloader\bootloader.bin 0x8000 build-v0-8m\partition_table\partition-table.bin 0x10000 build-v0-8m\KP-PTR_firmware.bin 0x190000 build-v0-8m\www.bin

idf.py -B build-v1-8m -DSDKCONFIG=build-v1-8m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_8m" build
REM nie dziala - esptool.py --chip ESP32S3 merge_bin -o build-v1-8m\merged-firmware.bin --flash_mode dio --flash_size 8MB 0x0 build-v1-8m\bootloader\bootloader.bin 0x8000 build-v1-8m\partition_table\partition-table.bin 0x10000 build-v1-8m\KP-PTR_firmware.bin 0x190000 build-v1-8m\www.bin

idf.py -B build-v1-16m -DSDKCONFIG=build-v1-16m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_16m" build
REM nie dziala - esptool.py --chip ESP32S3 merge_bin -o build-v1-16m\merged-firmware.bin --flash_mode dio --flash_size 16MB 0x0 build-v1-16m\bootloader\bootloader.bin 0x8000 build-v1-16m\partition_table\partition-table.bin 0x10000 build-v1-16m\KP-PTR_firmware.bin 0x190000 build-v1-16m\www.bin

idf.py -B build-v1-32m -DSDKCONFIG=build-v1-32m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_32m" build
eREM nie dziala - sptool.py --chip ESP32S3 merge_bin -o build-v1-32m\merged-firmware.bin --flash_mode dio --flash_size 32MB 0x0 build-v1-32m\bootloader\bootloader.bin 0x8000 build-v1-32m\partition_table\partition-table.bin 0x10000 build-v1-32m\KP-PTR_firmware.bin 0x190000 build-v1-32m\www.bin

@RD /S /Q Firmware
md Firmware 2>NULL

copy build-v0-8m/merged-firmware.bin Firmware/firmware-v0-8m.bin
copy build-v0-8m/KP-PTR_firmware.elf Firmware/firmware-v0-8m.elf

copy build-v1-8m/merged-firmware.bin Firmware/firmware-v1-8m.bin
copy build-v1-8m/KP-PTR_firmware.elf Firmware/firmware-v1-8m.elf

copy build-v1-16m/merged-firmware.bin Firmware/firmware-v1-16m.bin
copy build-v1-16m/KP-PTR_firmware.elf Firmware/firmware-v1-16m.elf

copy build-v1-32m/merged-firmware.bin Firmware/firmware-v1-32m.bin
copy build-v1-32m/KP-PTR_firmware.elf Firmware/firmware-v1-32m.elf


pause
