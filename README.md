# visionary

An ESP-IDF component library for the **SSD1306 OLED display** (128×64, I2C). Provides a framebuffer-based rendering API with text, pixel, and area operations on top of ESP-IDF's modern I2C master driver.

## Features

- Framebuffer-based rendering — draw offline, push to display in one shot
- `printf`-style text API with automatic line wrapping
- Pixel-level drawing via `visionary_set_pixel`
- Selective area clearing without touching the rest of the buffer
- **Flexible I2C ownership** — create a dedicated bus or attach to an existing one (great for multi-device setups)
- Proper resource cleanup via `visionary_deinit`
- Built on ESP-IDF's new `i2c_master` driver (ESP-IDF ≥ 5.x)

## Requirements

- ESP-IDF **v5.0 or later** (uses `driver/i2c_master.h`)
- SSD1306-based 128×64 OLED display on I2C

## Installation

Clone or copy this component into your project's `components/` directory:

```bash
cd your_project/components
git clone https://github.com/neobitlab/visionary.git
```

Then add it to your `CMakeLists.txt` if not already picked up automatically:

```cmake
set(EXTRA_COMPONENT_DIRS "components/visionary")
```

## Quick Start

```c
#include "visionary.h"

visionary_t display;

void app_main(void) {
    // Initialize with dedicated I2C bus on GPIO 22 (SCL) and GPIO 21 (SDA)
    ESP_ERROR_CHECK(visionary_init_new(&display, 22, 21));

    // Print formatted text at column 0, page 0
    visionary_display(&display, 0, 0, "Hello, %s!", "world");

    // Draw a pixel
    visionary_set_pixel(&display, 64, 32, true);
    visionary_update_screen(&display);

    // Clear a 30x8px area at column 10, page 2
    visionary_clear_area(&display, 10, 2, 30, 1);
    visionary_update_screen(&display);

    // Cleanup
    visionary_deinit(&display);
}
```

## Sharing an Existing I2C Bus

If you already have an I2C master bus (e.g. for another sensor), attach visionary to it instead of creating a new one:

```c
i2c_master_dev_handle_t my_existing_dev;
// ... your existing I2C setup ...

visionary_t display;
ESP_ERROR_CHECK(visionary_init_existing(&display, my_existing_dev));
```

`visionary_deinit` will **not** tear down the bus in this case — it only cleans up what it owns.

## API Reference

### Initialization

```c
// Create a new I2C master bus and add the SSD1306 device
esp_err_t visionary_init_new(visionary_t *display, uint8_t scl_pin, uint8_t sda_pin);

// Attach to an existing I2C master device handle
esp_err_t visionary_init_existing(visionary_t *display, i2c_master_dev_handle_t i2c_dev);

// Release resources (only deletes I2C bus if visionary created it)
esp_err_t visionary_deinit(visionary_t *display);
```

### Drawing

```c
// Write formatted text at pixel column x, page row y_page (y_page = pixel_y / 8)
// Automatically wraps to the next page if text overflows the line
esp_err_t visionary_display(visionary_t *display, uint8_t x, uint8_t y_page, const char *fmt, ...);

// Set or clear a single pixel (does not update screen — call visionary_update_screen after)
void visionary_set_pixel(visionary_t *display, uint8_t x, uint8_t y, bool on);

// Zero out a rectangular region in the framebuffer (does not update screen)
esp_err_t visionary_clear_area(visionary_t *display, uint8_t x, uint8_t y_page,
                                uint8_t width, uint8_t height);
```

### Buffer & Screen

```c
// Zero out the entire framebuffer (does not update screen)
void visionary_clear_buffer(visionary_t *display);

// Flush the framebuffer to the display over I2C
esp_err_t visionary_update_screen(visionary_t *display);

// Clear buffer + flush in one call
esp_err_t visionary_clear_display(visionary_t *display);
```

### Coordinate System

The SSD1306 organizes display memory in **pages**, where each page is 8 pixel rows tall:

```
Page 0 → pixels y=0..7
Page 1 → pixels y=8..15
Page 2 → pixels y=16..23
...
Page 7 → pixels y=56..63
```

The `visionary_display` and `visionary_clear_area` functions take `y_page` (0–7), not raw pixel Y. `visionary_set_pixel` takes raw pixel coordinates (0–127 for X, 0–63 for Y).

## Configuration

Default I2C address and display dimensions are defined in `include/defs.h`. To use a non-standard I2C address or a 128×32 display, modify those defines before building.
