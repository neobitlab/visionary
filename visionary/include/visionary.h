#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "driver/i2c_master.h"

/**
 * @brief Display context structure
 */
typedef struct {
    i2c_master_dev_handle_t i2c_dev;  // I2C device handle
    i2c_master_bus_handle_t bus_handle; // I2C bus handle (only used if owns_i2c is true)
    bool owns_i2c;                    // Whether this instance owns the I2C bus
    uint8_t buffer[VISIONARY_SCREEN_WIDTH * (VISIONARY_SCREEN_HEIGHT / 8)]; // Display buffer
} visionary_t;

/**
 * @brief Initialize the OLED display with new I2C bus.
 *
 * This function creates a new I2C bus and initializes the display.
 * Best for quick prototyping.
 *
 * @param display Pointer to visionary_t structure to initialize
 * @param scl_pin GPIO pin number for I2C SCL
 * @param sda_pin GPIO pin number for I2C SDA
 * @return ESP_OK on success, or an error code from the I2C driver on failure.
 */
esp_err_t visionary_init_new(visionary_t *display, uint8_t scl_pin, uint8_t sda_pin);

/**
 * @brief Initialize the OLED display with an existing I2C bus.
 *
 * This function initializes the display using an existing I2C bus.
 * Best for when you need to share the I2C bus with other devices.
 *
 * @param display Pointer to visionary_t structure to initialize
 * @param i2c_dev Existing I2C device handle
 * @return ESP_OK on success, or an error code from the I2C driver on failure.
 */
esp_err_t visionary_init_existing(visionary_t *display, i2c_master_dev_handle_t i2c_dev);

/**
 * @brief Deinitialize the  OLED display.
 *
 * This function cleans up resources. If the display owns the I2C bus,
 * it will also deinitialize the I2C bus.
 *
 * @param display Pointer to visionary_t structure to deinitialize
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t visionary_deinit(visionary_t *display);

/**
 * @brief Display formatted text on the OLED screen.
 *
 * This function provides a flexible way to display text on the screen, similar to printf.
 * It supports all standard printf format specifiers and allows positioning the text
 * anywhere on the screen.
 *
 * @param display Pointer to visionary_t structure
 * @param x Starting x-coordinate (0 to VISIONARY_SCREEN_WIDTH-1)
 * @param y_page Starting y-coordinate in pages (0 to 7)
 * @param fmt Format string (like printf)
 * @param ... Variable arguments for the format string
 * @return ESP_OK on success, or an error code if displaying fails.
 * 
 * @note The text will wrap to the next line if it exceeds the screen width.
 *       Each line is 8 pixels high (one page).
 * 
 * @example
 *   // Display a number at (0,0)
 *   visionary_display(display, 0, 0, "%d", 12345);
 *   
 *   // Display multiple values with labels
 *   visionary_display(display, 0, 1, "Temp: %.1f°C", temperature);
 *   visionary_display(display, 0, 2, "Hum: %d%%", humidity);
 *   
 *   // Display centered text (approximate)
 *   const char *text = "Hello World";
 *   int x = (VISIONARY_SCREEN_WIDTH - (strlen(text) * 6)) / 2;
 *   visionary_display(display, x, 3, "%s", text);
 */
esp_err_t visionary_display(visionary_t *display, uint8_t x, uint8_t y_page, const char *fmt, ...);

/**
 * @brief Clears the entire OLED display.
 *
 * @param display Pointer to visionary_t structure
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t visionary_clear_display(visionary_t *display);

/**
 * @brief Updates the display with the content of the internal buffer.
 *
 * @param display Pointer to visionary_t structure
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t visionary_update_screen(visionary_t *display);

/**
 * @brief Clears a specific area of the display buffer.
 *
 * This function clears a rectangular area of the display buffer without
 * affecting other areas. This is useful for updating specific parts of
 * the display without causing flickering.
 *
 * @param display Pointer to visionary_t structure
 * @param x Starting x-coordinate (0 to VISIONARY_SCREEN_WIDTH-1)
 * @param y_page Starting y-coordinate in pages (0 to 7)
 * @param width Width of area to clear in pixels
 * @param height Height of area to clear in pages
 * @return ESP_OK on success, or an error code if parameters are invalid.
 * 
 * @example
 *   // Clear first line of display
 *   visionary_clear_area(display, 0, 0, VISIONARY_SCREEN_WIDTH, 1);
 *   
 *   // Clear a specific area for sensor data
 *   visionary_clear_area(display, 0, 2, 20, 1);  // Clear 20 pixels wide area on third line
 */
esp_err_t visionary_clear_area(visionary_t *display, uint8_t x, uint8_t y_page, uint8_t width, uint8_t height);