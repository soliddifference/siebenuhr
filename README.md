# SIEBENUHR

### A museum-quality modern take on the classic 7-segment clock, enlarged to ridiculous proportions.

### Made from high quality materials, the gorgeous color provided by the latest in professional color-balanced LED illumination. 

###  More than just a clock, SEIBENUHR becomes a centerpiece for your home, providing feedback through color and animation. It’s able to react to motion, daytime, the weather and can even integrate seamlessly with Home Assistant to provide alarms and visual notifications.

![clock lit up](https://github.com/somebox/siebenuhr2022/raw/main/rsc/images/Siebenuhr_BW-006_framed.jpg)

## Building

### Prerequisites

- PlatformIO Core or PlatformIO IDE extension for VS Code
- Python 3.x (for PlatformIO)

### Build Steps

1. Clone the repository with its dependencies
2. Open the `siebenuhr` folder in PlatformIO
3. **Important**: Modify the ESPAsyncWiFiManager library dependency:
   - Navigate to `.pio/libdeps/esp32doit-devkit-v1/ESPAsyncWiFiManager/src/ESPAsyncWiFiManager.h`
   - Change the `AsyncWiFiManagerParameter` class member variables from `private` to `public` (around line 95-100)
   - This is required because the project customizes the captive portal UI and needs direct access to these members
4. Build the project:
   ```bash
   pio run
   ```
5. Upload to device:
   ```bash
   pio run --target upload
   ```

## Configuration

### Clock Models

SIEBENUHR supports different clock models with varying LED counts. You need to configure your build for your specific hardware:

#### 1. Clock Type Selection

Edit `src/siebenuhr.cpp` to set your clock type in the `setup()` function:

```cpp
// For MINI clock (4 LEDs per segment)
g_controller->initialize(siebenuhr_core::ClockType::CLOCK_TYPE_MINI);

// For REGULAR/BIG clock (11 or 17 LEDs per segment)
g_controller->initialize(siebenuhr_core::ClockType::CLOCK_TYPE_REGULAR);
```

#### 2. LED Count for Regular Clock

If you have a REGULAR clock, you also need to configure the LED count per segment in `siebenuhr_core/src/siebenuhr_core.h`:

```cpp
// For newer models with 17 LEDs per segment
constexpr int RegularLedsPerSegment = 17;

// For older models with 11 LEDs per segment
// constexpr int RegularLedsPerSegment = 11;
```

Comment/uncomment the appropriate line based on your hardware version.

#### 3. Default Personality

You can also set the default color personality in `src/siebenuhr.cpp`:

```cpp
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_SOLIDCOLOR);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_COLORWHEEL);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_RAINBOW);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_MOSAIK);
g_controller->setPersonality(siebenuhr_core::PersonalityType::PERSONALITY_GLITTER);
```

### Development Notes

When developing the library and the firmware at the same time, the lib_dependency needs to be updated manually with:

```cmd
pio lib update
```

