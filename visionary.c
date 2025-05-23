#include "visionary.h"
#include "defs.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static const char *TAG = "VISIONARY";

static esp_err_t visionary_send_cmd(visionary_t *display, uint8_t cmd) {
    if (!display || !display->i2c_dev) {
        ESP_LOGE(TAG, "Display not initialized!");
        return ESP_FAIL;
    }
    uint8_t M_PACKET_LEN = 2;
    uint8_t write_buf[M_PACKET_LEN];
    write_buf[0] = 0x00;
    write_buf[1] = cmd;
    return i2c_master_transmit(display->i2c_dev, write_buf, M_PACKET_LEN, 100);
}

static esp_err_t visionary_send_data(visionary_t *display, const uint8_t *data, size_t len) {
    if (!display || !display->i2c_dev) {
        ESP_LOGE(TAG, "Display not initialized!");
        return ESP_FAIL;
    }
    if (len == 0) return ESP_OK;

    uint8_t *buffer = malloc(len + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for data buffer");
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = 0x40;
    memcpy(buffer + 1, data, len);

    esp_err_t ret = i2c_master_transmit(display->i2c_dev, buffer, len + 1, 100);
    free(buffer);
    return ret;
}

static esp_err_t visionary_configure_display(visionary_t *display) {
    const uint8_t init_cmds[] = {
        VISIONARY_CMD_DISPLAY_OFF,
        VISIONARY_CMD_SET_DISPLAY_CLOCK_DIV, 0x80,
        VISIONARY_CMD_SET_MULTIPLEX, VISIONARY_SCREEN_HEIGHT - 1,
        VISIONARY_CMD_SET_DISPLAY_OFFSET, 0x00,
        VISIONARY_CMD_SET_START_LINE | 0x00,
        VISIONARY_CMD_CHARGE_PUMP, 0x14, 
        VISIONARY_CMD_MEMORY_MODE, 0x00, 
        VISIONARY_CMD_SEG_REMAP | 0x01,   
        VISIONARY_CMD_COM_SCAN_DEC,       
        VISIONARY_CMD_SET_COM_PINS, 0x12, 
        VISIONARY_CMD_SET_CONTRAST, 0xCF,
        VISIONARY_CMD_SET_PRECHARGE, 0xF1,
        VISIONARY_CMD_SET_VCOM_DETECT, 0x40,
        VISIONARY_CMD_DISPLAY_ALL_ON_RESUME,
        VISIONARY_CMD_NORMAL_DISPLAY,
        VISIONARY_CMD_DEACTIVATE_SCROLL,
        VISIONARY_CMD_DISPLAY_ON
    };

    for (size_t i = 0; i < sizeof(init_cmds); ++i) {
        esp_err_t ret = visionary_send_cmd(display, init_cmds[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send init command 0x%02X: %s", init_cmds[i], esp_err_to_name(ret));
            return ret;
        }
    }
    ESP_LOGI(TAG, "Visionary display successfully initialized with commands.");
    return ESP_OK;
}

esp_err_t visionary_init_new(visionary_t *display, uint8_t scl_pin, uint8_t sda_pin) {
    if (!display) return ESP_ERR_INVALID_ARG;

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = VISIONARY_I2C_PORT,
        .scl_io_num = scl_pin,
        .sda_io_num = sda_pin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &display->bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C new master bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VISIONARY_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(display->bus_handle, &dev_cfg, &display->i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus add device failed: %s", esp_err_to_name(ret));
        i2c_del_master_bus(display->bus_handle);
        display->i2c_dev = NULL;
        display->bus_handle = NULL;
        return ret;
    }

    display->owns_i2c = true;
    memset(display->buffer, 0, sizeof(display->buffer));
    ESP_LOGI(TAG, "I2C Master initialized, Visionary device added successfully.");
    
    return visionary_configure_display(display);
}

esp_err_t visionary_init_existing(visionary_t *display, i2c_master_dev_handle_t i2c_dev) {
    if (!display || !i2c_dev) return ESP_ERR_INVALID_ARG;
    
    display->i2c_dev = i2c_dev;
    display->bus_handle = NULL;
    display->owns_i2c = false;
    memset(display->buffer, 0, sizeof(display->buffer));
    
    return visionary_configure_display(display);
}

esp_err_t visionary_deinit(visionary_t *display) {
    if (!display) return ESP_ERR_INVALID_ARG;
    
    if (display->owns_i2c) {
        if (display->i2c_dev) {
            i2c_master_bus_rm_device(display->i2c_dev);
        }
        if (display->bus_handle) {
            i2c_del_master_bus(display->bus_handle);
        }
    }
    
    display->i2c_dev = NULL;
    display->bus_handle = NULL;
    display->owns_i2c = false;
    return ESP_OK;
}

void visionary_clear_buffer(visionary_t *display) {
    if (display) {
        memset(display->buffer, 0, sizeof(display->buffer));
    }
}

esp_err_t visionary_update_screen(visionary_t *display) {
    if (!display) return ESP_ERR_INVALID_ARG;

    esp_err_t ret;
    ret = visionary_send_cmd(display, VISIONARY_CMD_COLUMN_ADDR); if(ret != ESP_OK) return ret;
    ret = visionary_send_cmd(display, 0);                            if(ret != ESP_OK) return ret; // Column start address
    ret = visionary_send_cmd(display, VISIONARY_SCREEN_WIDTH - 1);     if(ret != ESP_OK) return ret; // Column end address

    ret = visionary_send_cmd(display, VISIONARY_CMD_PAGE_ADDR);   if(ret != ESP_OK) return ret;
    ret = visionary_send_cmd(display, 0);                            if(ret != ESP_OK) return ret; // Page start address
    ret = visionary_send_cmd(display, (VISIONARY_SCREEN_HEIGHT / 8) - 1); if(ret != ESP_OK) return ret; // Page end address

    ret = visionary_send_data(display, display->buffer, sizeof(display->buffer));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send display buffer: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t visionary_clear_display(visionary_t *display) {
    if (!display) return ESP_ERR_INVALID_ARG;
    visionary_clear_buffer(display);
    return visionary_update_screen(display);
}

void visionary_set_pixel(visionary_t *display, uint8_t x, uint8_t y, bool on) {
    if (!display || x >= VISIONARY_SCREEN_WIDTH || y >= VISIONARY_SCREEN_HEIGHT) {
        return;
    }
    uint16_t index = x + (y / 8) * VISIONARY_SCREEN_WIDTH;
    uint8_t bit_pos = y % 8;

    if (on) {
        display->buffer[index] |= (1 << bit_pos);
    } else {
        display->buffer[index] &= ~(1 << bit_pos);
    }
}

// Helper function to draw a single character
static void visionary_draw_char(visionary_t *display, char c, uint8_t x_col, uint8_t y_page) {
    if (!display || y_page >= (VISIONARY_SCREEN_HEIGHT / 8)) {
        ESP_LOGW(TAG, "Invalid parameters for draw_char");
        return;
    }
    if (c < 32 || c > 126) {
        c = '?';
    }
    const uint8_t* char_data = font5x8[c - 32];
    
    for (uint8_t i = 0; i < 5; i++) { 
        if (x_col + i >= VISIONARY_SCREEN_WIDTH) continue;
        
        uint8_t line_data = char_data[i];
        uint16_t buffer_index = (x_col + i) + (y_page * VISIONARY_SCREEN_WIDTH);

        if (buffer_index < sizeof(display->buffer)) {
            display->buffer[buffer_index] = line_data;
        } else {
            ESP_LOGW(TAG, "Buffer overflow attempt in draw_char at x:%d, page:%d", x_col+i, y_page);
        }
    }
}

esp_err_t visionary_display(visionary_t *display, uint8_t x, uint8_t y_page, const char *fmt, ...) {
    if (!display || !fmt || y_page >= (VISIONARY_SCREEN_HEIGHT / 8)) {
        ESP_LOGE(TAG, "Invalid parameters for display");
        return ESP_ERR_INVALID_ARG;
    }

    // Calculate required buffer size
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        ESP_LOGE(TAG, "Failed to calculate string length");
        return ESP_FAIL;
    }

    // Allocate buffer for the formatted string
    char *buffer = malloc(len + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for display buffer");
        return ESP_ERR_NO_MEM;
    }

    // Format the string
    va_start(args, fmt);
    vsnprintf(buffer, len + 1, fmt, args);
    va_end(args);

    // Draw the string
    uint8_t current_x = x;
    uint8_t current_y = y_page;
    const char *ptr = buffer;

    while (*ptr) {
        if (current_x + 5 >= VISIONARY_SCREEN_WIDTH) {
            // Move to next line
            current_x = x;  // Reset to original x position
            current_y++;
            if (current_y >= (VISIONARY_SCREEN_HEIGHT / 8)) {
                ESP_LOGW(TAG, "Text overflow on display");
                break;
            }
        }

        visionary_draw_char(display, *ptr, current_x, current_y);
        current_x += (5 + 1);  // 5 for char width, 1 for spacing
        ptr++;
    }

    free(buffer);
    return visionary_update_screen(display);
}

esp_err_t visionary_clear_area(visionary_t *display, uint8_t x, uint8_t y_page, uint8_t width, uint8_t height) {
    if (!display) {
        ESP_LOGE(TAG, "Invalid display pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Validate parameters
    if (x >= VISIONARY_SCREEN_WIDTH || 
        y_page >= (VISIONARY_SCREEN_HEIGHT / 8) || 
        width == 0 || height == 0 ||
        x + width > VISIONARY_SCREEN_WIDTH ||
        y_page + height > (VISIONARY_SCREEN_HEIGHT / 8)) {
        ESP_LOGE(TAG, "Invalid clear area parameters: x=%d, y=%d, w=%d, h=%d", 
                 x, y_page, width, height);
        return ESP_ERR_INVALID_ARG;
    }

    // Clear the specified area
    for (uint8_t page = y_page; page < y_page + height; page++) {
        for (uint8_t col = x; col < x + width; col++) {
            display->buffer[col + (page * VISIONARY_SCREEN_WIDTH)] = 0;
        }
    }

    return ESP_OK;
}

