#pragma once

#include <stdint.h>

// Command definitions
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

// Display configuration
#define VISIONARY_I2C_PORT          I2C_NUM_0 // Or I2C_NUM_1 if I2C_NUM_0 is used elsewhere
#define VISIONARY_I2C_ADDRESS       0x3C
#define VISIONARY_SCREEN_WIDTH      128
#define VISIONARY_SCREEN_HEIGHT     64

// 5x8 font data
extern const uint8_t font5x8[][5];
