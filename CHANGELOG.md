# Changelog

All notable changes to the Siebenuhr firmware will be documented in this file.

## [1.1.0] - 2026-01-09

Major refactor integrating the `siebenuhr_core` library for shared functionality with ESPHome.

### Added
- Integrated `siebenuhr_core` library (v1.1.0) for display, glyph, and personality rendering
- WiFi configuration via captive portal (Access Point mode)
- Persistent configuration using ESP32 Preferences (replaces raw EEPROM)
- NTP time synchronization with timezone support (ezTime)
- Render state machine: SPLASH → WIFI → NTP → CLOCK
- Configurable build flags in `platformio.ini`:
  - `VERBOSE_LOGGING` - enables DEBUG level log output
  - `POWER_MONITORING_ENABLED` - enables INA219 power sensor logging
  - `AUTO_BRIGHTNESS_ENABLED` - enables BH1750 ambient light auto-brightness
  - `SENSOR_READ_INTERVAL_MS` - configurable sensor polling interval
- Brightness rate limiting and logarithmic mapping for smoother encoder control
- Long-press reset to re-enter WiFi configuration mode
- Deferred configuration saving to reduce flash wear

### Changed
- Firmware now depends on `siebenuhr_core` for display rendering, personalities, and input handling
- Controller extends `BaseController` from core library
- Configuration storage migrated from raw EEPROM to ESP32 Preferences API
- Improved access point portal with better UX

### Fixed
- Hue button press bug when Access Point is active
- Default color mode not restored correctly after power/reset
- `VERBOSE_LOGGING` flag now correctly enables DEBUG output (was being reset by Logger::init)

## [1.0.0] - 2024-11-02

Initial public release.

### Features
- 7-segment LED clock display with multiple color personalities
- Support for MINI (4 LEDs/segment) and REGULAR (11-17 LEDs/segment) clock types
- Color personalities: Solid Color, Color Wheel, Rainbow, Mosaik, Glitter
- Rotary encoder for brightness and hue adjustment
- Button controls for personality switching
- WiFi connectivity with NTP time sync
