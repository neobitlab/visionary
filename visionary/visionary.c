#include "visionary.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "VISIONARY";

#define VISIONARY_CMD_SET_CONTRAST          0x81
#define VISIONARY_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define VISIONARY_CMD_DISPLAY_ALL_ON        0xA5
#define VISIONARY_CMD_NORMAL_DISPLAY        0xA6
#define VISIONARY_CMD_INVERT_DISPLAY        0xA7
#define VISIONARY_CMD_DISPLAY_OFF           0xAE
#define VISIONARY_CMD_DISPLAY_ON            0xAF
#define VISIONARY_CMD_SET_DISPLAY_OFFSET    0xD3
#define VISIONARY_CMD_SET_COM_PINS          0xDA
#define VISIONARY_CMD_SET_VCOM_DETECT       0xDB
#define VISIONARY_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define VISIONARY_CMD_SET_PRECHARGE         0xD9
#define VISIONARY_CMD_SET_MULTIPLEX         0xA8
#define VISIONARY_CMD_SET_LOW_COLUMN        0x00
#define VISIONARY_CMD_SET_HIGH_COLUMN       0x10
#define VISIONARY_CMD_SET_START_LINE        0x40
#define VISIONARY_CMD_MEMORY_MODE           0x20
#define VISIONARY_CMD_COLUMN_ADDR           0x21
#define VISIONARY_CMD_PAGE_ADDR             0x22
#define VISIONARY_CMD_COM_SCAN_INC          0xC0
#define VISIONARY_CMD_COM_SCAN_DEC          0xC8
#define VISIONARY_CMD_SEG_REMAP             0xA0
#define VISIONARY_CMD_CHARGE_PUMP           0x8D
#define VISIONARY_CMD_DEACTIVATE_SCROLL     0x2E

static i2c_master_dev_handle_t visionary_i2c_dev_handle = NULL;

static uint8_t s_visionary_buffer[VISIONARY_SCREEN_WIDTH * (VISIONARY_SCREEN_HEIGHT / 8)];

static const uint8_t font5x8[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // \  (backslash)
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x02, 0x01, 0x02, 0x04, 0x02}  // ~
};

static esp_err_t visionary_send_cmd(uint8_t cmd) {
    if (visionary_i2c_dev_handle == NULL) {
        ESP_LOGE(TAG, "I2C device handle not initialized!");
        return ESP_FAIL;
    }
    uint8_t M_PACKET_LEN = 2;
    uint8_t write_buf[M_PACKET_LEN];
    write_buf[0] = 0x00;
    write_buf[1] = cmd;
    return i2c_master_transmit(visionary_i2c_dev_handle, write_buf, M_PACKET_LEN, 100);
}

static esp_err_t visionary_send_data(const uint8_t *data, size_t len) {
    if (visionary_i2c_dev_handle == NULL) {
        ESP_LOGE(TAG, "I2C device handle not initialized!");
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

    esp_err_t ret = i2c_master_transmit(visionary_i2c_dev_handle, buffer, len + 1, 100);
    free(buffer);
    return ret;
}



    //IIC Configuration in here because it was easier to do it this way
    //Note to self: This is not the best way to do it, but it works for now :)

esp_err_t visionary_init(void) {
    esp_err_t ret;

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = VISIONARY_I2C_PORT,
        .scl_io_num = VISIONARY_I2C_SCL_GPIO,
        .sda_io_num = VISIONARY_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t bus_handle;
    ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C new master bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VISIONARY_I2C_ADDRESS,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &visionary_i2c_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus add device failed: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        visionary_i2c_dev_handle = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "I2C Master initialized, Visionary device added successfully.");

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
        ret = visionary_send_cmd(init_cmds[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send init command 0x%02X: %s", init_cmds[i], esp_err_to_name(ret));
            i2c_master_bus_rm_device(visionary_i2c_dev_handle);
            i2c_del_master_bus(bus_handle);
            visionary_i2c_dev_handle = NULL;
            return ret;
        }
    }
    ESP_LOGI(TAG, "Visionary display successfully initialized with commands.");

    return visionary_clear_display();
}

void visionary_clear_buffer(void) {
    memset(s_visionary_buffer, 0x00, sizeof(s_visionary_buffer));
}

esp_err_t visionary_update_screen(void) {
    esp_err_t ret;
    ret = visionary_send_cmd(VISIONARY_CMD_COLUMN_ADDR); if(ret != ESP_OK) return ret;
    ret = visionary_send_cmd(0);                            if(ret != ESP_OK) return ret; // Column start address
    ret = visionary_send_cmd(VISIONARY_SCREEN_WIDTH - 1);     if(ret != ESP_OK) return ret; // Column end address

    // Set page address range (0 to 7 for 64 rows)
    ret = visionary_send_cmd(VISIONARY_CMD_PAGE_ADDR);   if(ret != ESP_OK) return ret;
    ret = visionary_send_cmd(0);                            if(ret != ESP_OK) return ret; // Page start address
    ret = visionary_send_cmd((VISIONARY_SCREEN_HEIGHT / 8) - 1); if(ret != ESP_OK) return ret; // Page end address

    ret = visionary_send_data(s_visionary_buffer, sizeof(s_visionary_buffer));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send display buffer: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t visionary_clear_display(void) {
    visionary_clear_buffer();
    return visionary_update_screen();
}

// Set a single pixel in the buffer
// (x, y) are absolute pixel coordinates
void visionary_set_pixel(uint8_t x, uint8_t y, bool on) {
    if (x >= VISIONARY_SCREEN_WIDTH || y >= VISIONARY_SCREEN_HEIGHT) {
        return; // Out of bounds
    }
    // Calculate the index in the buffer and the bit position
    // The display buffer is organized in pages. Each page is 8 pixels high.
    // Index = x_coord + (y_coord / 8) * screen_width
    uint16_t index = x + (y / 8) * VISIONARY_SCREEN_WIDTH;
    uint8_t bit_pos = y % 8;

    if (on) {
        s_visionary_buffer[index] |= (1 << bit_pos);
    } else {
        s_visionary_buffer[index] &= ~(1 << bit_pos);
    }
}

// Draw a character at a given position (x_col, y_page)
// x_col is pixel column, y_page is page row (0-7)
void visionary_draw_char(char c, uint8_t x_col, uint8_t y_page) {
    if (y_page >= (VISIONARY_SCREEN_HEIGHT / 8)) {
         ESP_LOGW(TAG, "Invalid page %d for draw_char", y_page);
        return;
    }
    if (c < 32 || c > 126) { // ASCII 32-126 are in the font
        c = '?'; // Default for unknown characters (ASCII 63)
    }
    const uint8_t* char_data = font5x8[c - 32];
    
    for (uint8_t i = 0; i < 5; i++) { 
        if (x_col + i >= VISIONARY_SCREEN_WIDTH) continue;
        
        uint8_t line_data = char_data[i];
        uint16_t buffer_index = (x_col + i) + (y_page * VISIONARY_SCREEN_WIDTH);

        if (buffer_index < sizeof(s_visionary_buffer)) {
             s_visionary_buffer[buffer_index] = line_data;
        } else {
            ESP_LOGW(TAG, "Buffer overflow attempt in draw_char at x:%d, page:%d", x_col+i, y_page);
        }
    }
}


// Draw a string at a given position (x_col, y_page)
esp_err_t visionary_draw_string(const char* text, uint8_t x, uint8_t y_page) {
    if (y_page >= (VISIONARY_SCREEN_HEIGHT / 8)) {
        ESP_LOGE(TAG, "Invalid page %d for draw_string", y_page);
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t current_x = x;
    while (*text) {
        if (current_x + 5 >= VISIONARY_SCREEN_WIDTH) { 
            ESP_LOGW(TAG, "Text overflow on draw_string");
            break;
        }
        visionary_draw_char(*text, current_x, y_page);
        current_x += (5 + 1); // 5 for char width, 1 for spacing between chars
        text++;
    }
    return ESP_OK;
}


esp_err_t display_data_oled(uint32_t value) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)value);

    visionary_clear_buffer();

    esp_err_t ret = visionary_draw_string(buffer, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to draw string for value %lu", (unsigned long)value);
        return ret;
    }

    return visionary_update_screen();
}

