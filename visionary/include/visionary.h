#ifndef VISIONARY
#define VISIONARY

#include <stdint.h>
#include <esp_err.h>

// Display configuration
#define VISIONARY_I2C_SCL_GPIO      8
#define VISIONARY_I2C_SDA_GPIO      9 
#define VISIONARY_I2C_PORT          I2C_NUM_0 // Or I2C_NUM_1 if I2C_NUM_0 is used elsewhere
#define VISIONARY_I2C_ADDRESS       0x3C
#define VISIONARY_SCREEN_WIDTH      128
#define VISIONARY_SCREEN_HEIGHT     64

/**
 * @brief Initialize the VISIONARY OLED display.
 *
 * This function initializes the I2C communication and sends the necessary
 * commands to configure the VISIONARY display.
 *
 * @return esp_err_t ESP_OK on success, or an error code from the I2C driver on failure.
 */
esp_err_t visionary_init(void);

/**
 * @brief Display a uint32_t value on the OLED.
 *
 * This function clears the display, converts the given unsigned 32-bit integer
 * to a string, and displays it on the OLED screen using the default font.
 * The text is displayed starting from the top-left corner (0,0).
 *
 * @param value The uint32_t value to display.
 * @return esp_err_t ESP_OK on success, or an error code if displaying fails.
 */
esp_err_t display_data_oled(uint32_t value);

/**
 * @brief Clears the entire OLED display.
 *
 * Fills the internal buffer with 0s and updates the screen.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t visionary_clear_display(void);

/**
 * @brief Draws a string at the specified coordinates.
 *
 * @param text The null-terminated string to draw.
 * @param x The x-coordinate to start drawing.
 * @param y The y-coordinate (page, 0-7) to start drawing.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t visionary_draw_string(const char* text, uint8_t x, uint8_t y_page);

/**
 * @brief Updates the display with the content of the internal buffer.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t visionary_update_screen(void);

#endif