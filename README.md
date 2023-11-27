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
