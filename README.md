# Siebenuhr

[![Tests](https://github.com/soliddifference/siebenuhr/actions/workflows/test.yml/badge.svg)](https://github.com/soliddifference/siebenuhr/actions/workflows/test.yml)

> This repository contains the open-source firmware for the Siebenuhr wall clock and mini clock. If you're using Home Assistant, we have [an ESPHome integration](https://github.com/soliddifference/siebenuhr_esphome). Both can be ordered from the web shop at [soliddifference.com](https://soliddifference.com/).

## The Clocks

Made from high quality materials with gorgeous color provided by professional color-balanced LED illumination. More than just a clock, Siebenuhr becomes a centerpiece for your home, providing feedback through color and animation.

### **Siebenuhr MK1**

A museum-quality modern take on the classic 7-segment clock, enlarged to ridiculous proportions.

![clock lit up](/hardware/mk1.jpg)

### **Siebenuhr Mini**

A smaller version of Siebenuhr, perfect for desks and bedside tables.

![clock lit up](/hardware/miniclock.jpg)

## Requirements

- ESP32 (tested with ESP32-DOIT-DEVKIT-V1)
- PlatformIO Core or PlatformIO IDE extension for VS Code
- Python 3.x

## Quick Start

```bash
# Clone the repository
git clone https://github.com/soliddifference/siebenuhr.git
cd siebenuhr

# For development: clone siebenuhr_core into tmp/
git clone https://github.com/soliddifference/siebenuhr_core.git tmp/siebenuhr_core

# Build for Mini clock
pio run -e esp32-mini

# Build for Regular clock
pio run -e esp32-regular

# Upload to device (replace with your environment)
pio run -e esp32-mini -t upload

# Monitor serial output
pio device monitor
```

## Configuration

### Clock Type

Select your clock type by building the appropriate environment:

```bash
# Mini clock (4 LEDs per segment, 112 total)
pio run -e esp32-mini

# Regular clock (17 LEDs per segment, 476 total)
pio run -e esp32-regular
```

Each environment sets the appropriate `CLOCK_TYPE_MINI` or `CLOCK_TYPE_REGULAR` build flag.

### Build Flags

Configure features in `platformio.ini` under the appropriate environment:

```ini
build_flags = 
    -D BUILD_CLOCK_MINI=1                  ; or BUILD_CLOCK_REGULAR=1
    -D SENSOR_READ_INTERVAL_MS=10000       ; Sensor polling interval (ms)
    -D VERBOSE_LOGGING=1                   ; Enable DEBUG level logging
    -D POWER_MONITORING_ENABLED=1          ; Enable INA219 power logging
    ; -D AUTO_BRIGHTNESS_ENABLED=1         ; Enable BH1750 auto-brightness
```

| Flag | Default | Description |
|------|---------|-------------|
| `BUILD_CLOCK_MINI` | - | Build for Mini clock (4 LEDs/segment) |
| `BUILD_CLOCK_REGULAR` | - | Build for Regular clock (17 LEDs/segment) |
| `SENSOR_READ_INTERVAL_MS` | 10000 | How often to read I2C sensors |
| `VERBOSE_LOGGING` | off | Enable DEBUG level log output |
| `POWER_MONITORING_ENABLED` | off | Log voltage/current from INA219 |
| `AUTO_BRIGHTNESS_ENABLED` | off | Adjust brightness from BH1750 ambient light |

### Display Personalities

Set the default personality in `src/siebenuhr.cpp`:

```cpp
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_SOLIDCOLOR);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_RAINBOW);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_MOSAIK);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_GLITTER);
```

## WiFi Setup

On first boot (or after long-press reset), the clock creates an access point named `SiebenuhrAP`. Connect to it and configure your WiFi credentials and timezone via the captive portal at `http://192.168.4.1`.

## Development

### Setup

```bash
# Install PlatformIO
pip install -r requirements.txt
```

### Project Structure

```
siebenuhr/
├── src/                  # Firmware source
│   ├── siebenuhr.cpp     # Main entry point
│   ├── Controller.*      # Clock controller (extends siebenuhr_core)
│   ├── configuration.*   # Persistent settings (ESP32 Preferences)
│   ├── accesspoint.*     # WiFi captive portal
│   └── timezone.h        # Timezone definitions
├── test/                 # Native unit tests
├── tmp/                  # Local siebenuhr_core (gitignored)
└── platformio.ini
```

### Build Environments

| Environment | Clock Type | LEDs/Segment | Total LEDs |
|-------------|------------|--------------|------------|
| `esp32-mini` | Mini | 4 | 112 |
| `esp32-regular` | Regular | 17 | 476 |
| `native` | - | - | Unit tests only |

### Dependencies

This firmware depends on [siebenuhr_core](https://github.com/soliddifference/siebenuhr_core) for display rendering, personalities, and input handling.

**For development** (local library):
```ini
lib_extra_dirs = tmp
```

**For release** (from GitHub):
```ini
lib_deps += https://github.com/soliddifference/siebenuhr_core.git#v1.1.0
```

### Running Tests

Unit tests run on your computer (not on ESP32):

```bash
# Run all tests
pio test -e native

# Run specific test suite
pio test -e native -f test_configuration

# Verbose output
pio test -e native -v
```

## Version

Current version: **1.1.0** (defined in `src/Controller.h`)

See [CHANGELOG.md](CHANGELOG.md) for release history.

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions welcome! Please fork and submit a pull request.
