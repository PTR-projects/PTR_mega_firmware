# Extra SPI chip-select lines

## Problem

ESP32-S3 limits hardware chip-select lines per SPI host to **6** (`CONFIG_SOC_SPI_MAX_CS_NUM=6`). That applies when CS is driven by the SPI peripheral via `spics_io_num` in `SPI_registerDevice()`.

On PTR mega v0.1 we already use six devices on `SPI2_HOST` (MS5607, LIS331, LSM6DSO32, SPI flash, MMC5983MA, SX1262). Adding another SPI device will hit this limit.

## Recommended solution: software CS

Register devices with `spics_io_num = -1` and assert/deassert a GPIO yourself around each transfer. Bus access is already serialized with `spi_device_acquire_bus` / `spi_device_release_bus` in `SPI_transfer()`.

```c
// register: spics_io_num = -1, store CS gpio in a small table keyed by handle
// transfer:
gpio_set_level(cs, 0);
spi_device_polling_transmit(...);
gpio_set_level(cs, 1);
```

**Pros**
- Unlimited CS count (limited only by free GPIOs)
- One shared bus and clocks
- Change stays localized in `SPI_driver`; sensor APIs can stay the same

**Cons**
- Slightly less precise CS timing than hardware CS
- Must guarantee idle-high on all CS at init and never leave CS low on error paths

This is the usual ESP-IDF approach once you need more than six devices on one host.

## Alternatives (usually worse here)

| Approach | When it makes sense | Downside |
|----------|---------------------|----------|
| Second SPI host (`SPI3`) | Electrically separate bus / high-speed group | Needs more pins; more software |
| External CS mux / decoder | Very tight on GPIOs | Extra HW, slower select, board spin |
| Move some parts off SPI (I2C, etc.) | Sensor supports it | Not always possible (flash, SX1262) |

## Practical migration path

1. Keep hardware CS until device #7 is needed.
2. When needed, switch **all** devices on that bus to software CS (mixing HW/SW CS is possible but messier).
3. In `SPI_init` / register: configure each CS as output, idle high (pull-up optional).
4. Optionally add short CS setup/hold delays if a part is picky (typical NOR flash is fine).

**Bottom line:** prefer extending `SPI_driver` with software chip-select so CS count tracks free GPIOs, not `SOC_SPI_MAX_CS_NUM`. Do not add a mux or second bus until software CS is insufficient.
