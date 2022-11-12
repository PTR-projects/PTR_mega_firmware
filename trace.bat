SET mypath=%~dp0
echo %mypath%

start "" %IDF_TOOLS_PATH%\tools\openocd-esp32\v0.11.0-esp32-20220411\openocd-esp32\bin\openocd.exe -s %IDF_TOOLS_PATH%\tools\openocd-esp32\v0.11.0-esp32-20220411\openocd-esp32\share\openocd\scripts -f interface/esp_usb_jtag.cfg -f target/esp32s3.cfg
timeout /t 5
start "" %IDF_TOOLS_PATH%\tools\xtensa-esp32s3-elf\esp-2021r2-patch3-8.4.0\xtensa-esp32s3-elf\bin\xtensa-esp32s3-elf-gdb.exe -x %mypath%gdbinit %mypath%build/KP-PTR_firmware.elf