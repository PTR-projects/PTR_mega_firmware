# KPPTR-firmware

Welcome to the KPPTR-firmware repository! This repository contains the firmware code for an on-board computer designed to enhance the capabilities of DIY amateur rockets and high-altitude balloons. The computer is powered by the ESP32-S3 SoC, providing the necessary computational power for various tasks during the flight. Both hardware and sofware was developed by PTR member!

## Project Overview

The main branch of this repository currently houses the firmware code written in C, utilizing the ESP-IDF 4.4 framework. To compile the software, you'll need the ESP-IDF 4.4 framework, which you can download from [here](https://dl.espressif.com/dl/esp-idf/?idf=4.4).

## Main features
1. 4 independent igniters with connectivity check indicated by individual LED. Safety fuse with reset included.
   2 outputs dedicated to parachute release. Other 2 user configurable.
2. Improved reliability. Duplicated [IMU](https://en.wikipedia.org/wiki/Inertial_measurement_unit) and onboard magnetometer.
3. Wide range of measured acceleration (1 - 400g).
4. Accurate altitude readings up to 11km with precision barometer.
5. High frequency (100Hz) data probing and logging.
6. Remote access via onboard WiFi access point and web interface.
7. Accurate and reliable position reporting read from multiple [GNSS systems](https://en.wikipedia.org/wiki/Gnss). Build in antenna.
8. Real time position tracking and access to onboard telemetry. Delivered via standard radio transmitter (Lora 433MHz)
9. 3 extension slots. 1 dedicated to servo or other [S.BUS compatible peripheral](https://www.futabarc.com/sbus/).
10. Detection of rocket flight stages for second stage ignition or parachute deployment.

## Software Components

This project is composed of several distinct software components, each responsible for a specific aspect of the on-board computer's functionality. Here's a brief overview of each component:

- **AHRS_driver**: This computing engine is tasked with processing data related to attitude, altitude, velocity, and more, providing crucial insights during the flight.
- **Analog_driver**: Responsible for handling the Analog-to-Digital Conversion (ADC) process, enabling measurements of Vbat (battery voltage) and the continuity of igniters.
- **BOARD**: This component defines board-specific configurations, ensuring seamless integration of the firmware with the hardware.
- **DataManager**: Efficiently packs data into Flash and RF frames, facilitating high-speed communication between the AHRS task and the Storage task.
- **esp_littlefs**: An external LittleFS library, augmenting file system capabilities for the project.
- **FLASH_driver**: While not currently used, this component is reserved for potential future integration with external Flash memory.
- **FlightStateDetector**: Detects the current flight state, contributing to accurate decision-making during the mission.
- **GNSS_driver**: Manages communication with the GNSS receiver, gathering essential location data.
- **IGN_driver**: Drives igniter outputs, crucial for controlled actions during the flight.
- **LED_driver**: Takes charge of LED (both standard and addressable) and buzzer control, aiding in visual and auditory signaling.
- **LIS331_driver**: Handles communication with LIS331 family acceleration sensors, vital for monitoring acceleration data.
- **LORA_driver**: Configures and facilitates data transmission using the LORA module and the provided SX126x_driver.
- **LSM6DSO32_driver**: Communicates with one or more LSM6DSO32 acceleration and gyro sensors, contributing to accurate motion tracking.
- **MMC5983MA_driver**: Manages communication with the MMC5983MA magnetometer sensor, essential for tracking magnetic fields.
- **MS5607_driver**: Establishes communication with the MS5607 pressure sensor, providing data about atmospheric pressure changes.
- **OpenLog_driver**: Currently not in use, this component is reserved for potential future utilization with an external blackbox.
- **Preferences**: Enables the application and modification of settings stored in Flash memory.
- **Sensors**: Serves as a higher-level component that utilizes drivers from various sensors, ensuring coordinated functionality.
- **Servo_driver**: Reserved for potential future use with servo motors.
- **soc**: This is a copy of the IDF component with applied fixes in the SPI driver.
- **SPI_driver**: Provides a custom API for the SPI peripheral, enhancing communication capabilities.
- **Storage_driver**: Handles data storage in Flash memory, ensuring important data is retained for later analysis.
- **SX126x_driver**: A library for the LORA module provided by the manufacturer, simplifying LORA communication.
- **SysMgr**: Acts as the system manager, monitoring the states of critical components to ensure reliable operation.
- **Telemetry_driver**: Currently not used, this component is reserved for potential future use.
- **Web_driver**: Manages the Web GUI, providing a user-friendly interface for interacting with the on-board computer.

Feel free to explore the individual components and tasks within the firmware to gain a deeper understanding of how each part contributes to the overall functionality of the on-board computer.

## Getting Started

### Windows
To start using the KPPTR-firmware and exploring its capabilities, follow these steps:

1. Clone the repository to your local machine.
2. Download the ESP-IDF 4.4 framework from [here](https://dl.espressif.com/dl/esp-idf/?idf=4.4).
3. Use the ESP-IDF tools to compile and flash the firmware onto your ESP32-S3 SoC.

Please check [ESP-IDF docs](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for getting started instructions.

### MacOS
As of Dec 2023 MacOS is supported with [command line esp-idf](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32s3/get-started/index.html#id3)
toolchain and [VCS extension](https://github.com/espressif/vscode-esp-idf-extension). In order to install cmd-line tooling follow steps below:
1. Add `cmake` and `pyenv`, install latest python

   ```bash
   $ brew install cmake pyenv ninja dfu-util
   $ pyenv install 3.11.6
   ```

2. Install `esp-idf` from [branch 4.4.x](https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/stable/get-started/index.html#linux-and-macos). 
   As of Dec 2023 branch 5.x is not yet supported by `KPPTR`.
3. Enter directory where you cloned toolchain and setup `esp-idf` env variables:
   ```bash
   $ source ./export.sh
   ```
4. Enter directory where you cloned `PTR_mega_firmware` and build project with:
   ```bash
   $ idf.py build
   ```
At the end you should see something along the lines:
```bash
[1134/1135] Generating binary image from built executable
esptool.py v3.3.4-dev
Creating esp32s3 image...
Merged 2 ELF sections
Successfully created esp32s3 image.
Generated /Users/[...]/PTR_mega_firmware/build/KP-PTR_firmware.bin
[1135/1135] cd /Users/[...]/PTR_mega_firmware/build/esp-idf/esp...le.bin /Users/[...]/PTR_mega_firmware/build/KP-PTR_firmware.bin
KP-PTR_firmware.bin binary size 0xaee70 bytes. Smallest app partition is 0x180000 bytes. 0xd1190 bytes (54%) free.

Project build complete. To flash, run this command:
/Users/[...]/.espressif/python_env/idf4.4_py3.11_env/bin/python ../esp-idf/components/esptool_py/esptool/esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32s3  write_flash --flash_mode dout --flash_size detect --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/KP-PTR_firmware.bin 0x190000 build/www.bin
or run 'idf.py -p (PORT) flash'
```

### Troubleshooting
1. Make sure you checked out `4.4.x` esp-idf branch. Branch `5.x` is not supported yet.  

## Hardware
### Prototype PCB
Hardware fot KPPTR is developed in repository [PTR_tracker_hardware](https://github.com/PTR-projects/PTR_tracker_hardware). 
As of Dec 2023 boards are not in serial production yet. Prototype design is almost fully tested, but still, it's a prototype. 

### Bread board
Not all development require dedicated PCB. As we use standard ESP32-S3 processor, development boards could be used. 

#### ESP32-S3-WROOM-1-N16R8
One of them is `ESP32-S3-WROOM-1-N16R8`. It can be flashed directly via USB cable without additional programmer. Only two
adjustments needs to be done. Run in `PTR_mega_firmware` directory command `idf.py menuconfig` and using menu:
1. Set partition table definition to match 16MB flash: `Serial flasher config -> flash size -> 16MB`
2. Set flash size to 16mb in project settings: `Partition table -> custom partition CSV file -> partitions-16mb.csv` 
3. Quit menu saving changes
4. Rebuild project

## Flashing firmware
1. Attach ESP32 board with USB cable to computer. `pwr` led should turn on.
2. Holding `rst` button press once `boot` button. This will initiate programming sequence on ESP32 side.
3. Find how your system recognise USB attached ESP32 board:
    ```bash
    # MacOS
    $ ls -l /dev/cu.*
    crw-rw-rw-  1 root  wheel  0x9000001 Nov 26 21:06 /dev/cu.usbmodem21401
    ```
    ```bash
    # Linux
    $ ls -l /dev/ttyUSB*
    crw-rw---- 1 root dialout 188, 0 Nov 23 21:05 /dev/ttyUSB0
    ```
4. Use USB device to flash firmware using command:
    ```bash
    $ idf.py -p <serial_port_name> -b 9600 flash monitor
    ```

    As a result you should see similar output (trimmed for brevity):   

    ```bash 
    Executing action: flash
    [...]
    esptool.py v3.3.4-dev
    Serial port /dev/cu.usbmodem21401
    Connecting...
    Chip is ESP32-S3 (revision v0.2)
    Features: WiFi, BLE
    Crystal is 40MHz
    MAC: de:ad:be:ef:1c:10
    Uploading stub...
    Running stub...
    Stub running...
    Configuring flash size...
    Flash will be erased from 0x00000000 to 0x00005fff...
    Flash will be erased from 0x00010000 to 0x000befff...
    Flash will be erased from 0x00008000 to 0x00008fff...
    Flash will be erased from 0x00190000 to 0x0020ffff...
    Compressed 22320 bytes to 13906...
    Writing at 0x00000000... (100 %)
    Wrote 22320 bytes (13906 compressed) at 0x00000000 in 0.2 seconds (effective 747.1 kbit/s)...
    Hash of data verified.
    Compressed 716400 bytes to 466476...
    Writing at 0x00010000... (3 %)
    [...]
    Wrote 524288 bytes (10053 compressed) at 0x00190000 in 2.6 seconds (effective 1607.7 kbit/s)...
    Hash of data verified.
    
    Leaving...
    Hard resetting via RTS pin...
    Executing action: monitor
    Running idf_monitor in directory /Users/[...]/PTR_mega_firmware
    [...]
    --- idf_monitor on /dev/cu.usbmodem21401 115200 ---
    --- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
    ESP-ROM:esp32s3-20210327
    Build:Mar 27 2021
    [...]
    I (24) boot: ESP-IDF v4.4.6 2nd stage bootloader
    I (25) boot: compile time 19:03:36
    I (25) boot: Multicore bootloader
    I (27) boot: chip revision: v0.2
    [...]
    I (0) cpu_start: App cpu up.
    I (294) cpu_start: Pro cpu start user code
    I (294) cpu_start: cpu freq: 240000000
    I (295) cpu_start: Application information:
    I (295) cpu_start: Project name:     KP-PTR_firmware
    I (295) cpu_start: App version:      1458fe8-dirty
    I (295) cpu_start: Compile time:     Nov 26 2023 19:03:32
    I (295) cpu_start: ELF file SHA256:  e8554e562adc0f10...
    I (296) cpu_start: ESP-IDF:          v4.4.6
    I (296) cpu_start: Min chip rev:     v0.0
    I (296) cpu_start: Max chip rev:     v0.99
    I (296) cpu_start: Chip rev:         v0.2
    I (296) heap_init: Initializing. RAM available for dynamic allocation:
    I (297) heap_init: At 3FCA9960 len 0003FDB0 (255 KiB): D/IRAM
    I (297) heap_init: At 3FCE9710 len 00005724 (21 KiB): STACK/DIRAM
    I (297) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
    [...]
    I (3700) KP-PTR: Task Main - ready!
    ```
5. Once started you should be able to join default WiFi network `KPPTR` with default password `MeteorPTR`. 
On address http://192.168.4.1/ listens onboard web server. Congrats, you started and verified running firmware \o/
## Contributors

- [bartekM](https://gitlab.com/space.tech)
- [https://gitlab.com/andrewziebinski](https://gitlab.com/andrewziebinski)
- [https://gitlab.com/Bdabrowsky](https://gitlab.com/Bdabrowsky)
- [https://gitlab.com/krecik1](https://gitlab.com/krecik1)
- [https://gitlab.com/US_air_force](https://gitlab.com/US_air_force)
- [https://gitlab.com/karmelek96](https://gitlab.com/karmelek96)
- and many others!

## License

This project is licensed under the [MIT License](LICENSE). You can freely use, modify, and distribute the code in accordance with the terms of the license.

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.

Thank you for your interest in KPPTR-firmware! If you have any questions or feedback, please don't hesitate to reach out to us. Happy coding and safe flights!
