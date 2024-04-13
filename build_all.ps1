if (-not $env:PYTHONNOUSERSITE) {
    Write-Host "Run from ESP-IDF console!"
    pause
    exit
}

Remove-Item -Path build-v0-8m            -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-8m            -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-16m           -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-32m           -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-arecorder-v3-8m  -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-arecorder-v3-32m -Recurse -Force -ErrorAction SilentlyContinue

idf.py -B build-v0-8m -DSDKCONFIG=build-v0-8m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v0_8m" build
esptool.py --chip ESP32S3 merge_bin -o build-v0-8m\merged-firmware.bin --flash_mode dio --flash_size 8MB 0x0 build-v0-8m\bootloader\bootloader.bin 0x8000 build-v0-8m\partition_table\partition-table.bin 0x10000 build-v0-8m\KP-PTR_firmware.bin 0x190000 build-v0-8m\www.bin

idf.py -B build-v1-8m -DSDKCONFIG=build-v1-8m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_8m" build
esptool.py --chip ESP32S3 merge_bin -o build-v1-8m\merged-firmware.bin --flash_mode dio --flash_size 8MB 0x0 build-v1-8m\bootloader\bootloader.bin 0x8000 build-v1-8m\partition_table\partition-table.bin 0x10000 build-v1-8m\KP-PTR_firmware.bin 0x190000 build-v1-8m\www.bin

idf.py -B build-v1-16m -DSDKCONFIG=build-v1-16m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_16m" build
esptool.py --chip ESP32S3 merge_bin -o build-v1-16m\merged-firmware.bin --flash_mode dio --flash_size 16MB 0x0 build-v1-16m\bootloader\bootloader.bin 0x8000 build-v1-16m\partition_table\partition-table.bin 0x10000 build-v1-16m\KP-PTR_firmware.bin 0x190000 build-v1-16m\www.bin

idf.py -B build-v1-32m -DSDKCONFIG=build-v1-32m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_v1_32m" build
esptool.py --chip ESP32S3 merge_bin -o build-v1-32m\merged-firmware.bin --flash_mode dio --flash_size 32MB 0x0 build-v1-32m\bootloader\bootloader.bin 0x8000 build-v1-32m\partition_table\partition-table.bin 0x10000 build-v1-32m\KP-PTR_firmware.bin 0x190000 build-v1-32m\www.bin

idf.py -B build-arecorder-v3-8m -DSDKCONFIG=build-arecorder-v3-8m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_arecorder_v3_8m" build
esptool.py --chip ESP32S3 merge_bin -o build-arecorder-v3-8m\merged-firmware.bin --flash_mode dio --flash_size 8MB 0x0 build-arecorder-v3-8m\bootloader\bootloader.bin 0x8000 build-arecorder-v3-8m\partition_table\partition-table.bin 0x10000 build-arecorder-v3-8m\KP-PTR_firmware.bin 0x190000 build-arecorder-v3-8m\www.bin

idf.py -B build-arecorder-v3-32m -DSDKCONFIG=build-arecorder-v3-32m/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig;sdkconfig_arecorder_v3_32m" build
esptool.py --chip ESP32S3 merge_bin -o build-arecorder-v3-32m\merged-firmware.bin --flash_mode dio --flash_size 32MB 0x0 build-arecorder-v3-32m\bootloader\bootloader.bin 0x8000 build-arecorder-v3-32m\partition_table\partition-table.bin 0x10000 build-arecorder-v3-32m\KP-PTR_firmware.bin 0x190000 build-arecorder-v3-32m\www.bin

Remove-Item -Path Firmware -Recurse -Force -ErrorAction SilentlyContinue
New-Item -Path Firmware -ItemType Directory > $null

Copy-Item -Path build-v0-8m/merged-firmware.bin -Destination Firmware/firmware-v0-8m.bin
Copy-Item -Path build-v0-8m/KP-PTR_firmware.elf -Destination Firmware/firmware-v0-8m.elf

Copy-Item -Path build-v1-8m/merged-firmware.bin -Destination Firmware/firmware-v1-8m.bin
Copy-Item -Path build-v1-8m/KP-PTR_firmware.elf -Destination Firmware/firmware-v1-8m.elf

Copy-Item -Path build-v1-16m/merged-firmware.bin -Destination Firmware/firmware-v1-16m.bin
Copy-Item -Path build-v1-16m/KP-PTR_firmware.elf -Destination Firmware/firmware-v1-16m.elf

Copy-Item -Path build-v1-32m/merged-firmware.bin -Destination Firmware/firmware-v1-32m.bin
Copy-Item -Path build-v1-32m/KP-PTR_firmware.elf -Destination Firmware/firmware-v1-32m.elf

Copy-Item -Path build-arecorder-v3-8m/merged-firmware.bin -Destination Firmware/firmware-arecorder-v3-8m.bin
Copy-Item -Path build-arecorder-v3-8m/KP-PTR_firmware.elf -Destination Firmware/firmware-arecorder-v3-8m.elf

Copy-Item -Path build-arecorder-v3-32m/merged-firmware.bin -Destination Firmware/firmware-arecorder-v3-32m.bin
Copy-Item -Path build-arecorder-v3-32m/KP-PTR_firmware.elf -Destination Firmware/firmware-arecorder-v3-32m.elf

Remove-Item -Path build-v0-8m  -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-8m  -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-16m -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-v1-32m -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-arecorder-v3-8m -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path build-arecorder-v3-32m -Recurse -Force -ErrorAction SilentlyContinue

pause
