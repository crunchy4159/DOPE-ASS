/**
 * @file dope_ass_config.h
 * @brief Project-wide compile-time constants for DOPE-ASS firmware.
 *
 * Pin assignments, I2C/SPI bus identifiers, RTOS task priorities, stack
 * sizes, and sensor polling rates.  All values are for the ESP32-P4
 * target; the native build may override where necessary.
 *
 * DOPE-ASS SRS v0.1 — §2 (Hardware Platform)
 */

#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Version
// ---------------------------------------------------------------------------
#ifndef DOPE_ASS_VERSION_MAJOR
#define DOPE_ASS_VERSION_MAJOR 0
#endif
#ifndef DOPE_ASS_VERSION_MINOR
#define DOPE_ASS_VERSION_MINOR 1
#endif

// ---------------------------------------------------------------------------
// I2C Bus 0: Sensors  (BMP581, RM3100, Rotary Encoder)
// ---------------------------------------------------------------------------
constexpr int     DASS_I2C0_PORT       = 0;
constexpr int     DASS_I2C0_SDA_PIN    = 8;       // GPIO — update per PCB
constexpr int     DASS_I2C0_SCL_PIN    = 9;       // GPIO — update per PCB
constexpr int     DASS_I2C0_FREQ_HZ    = 400000;  // 400 kHz Fast-mode

// I2C device addresses (7-bit)
constexpr uint8_t DASS_BMP581_ADDR     = 0x47;    // SDO→VDD
constexpr uint8_t DASS_RM3100_ADDR     = 0x20;    // SA0/1 low
constexpr uint8_t DASS_ENCODER_ADDR    = 0x36;    // AS5600 default

// ---------------------------------------------------------------------------
// SPI Bus: IMU  (ISM330DHCX)
// ---------------------------------------------------------------------------
constexpr int     DASS_SPI_HOST        = 2;        // SPI3_HOST on ESP32-P4
constexpr int     DASS_SPI_MOSI_PIN    = 11;       // GPIO — update per PCB
constexpr int     DASS_SPI_MISO_PIN    = 13;       // GPIO — update per PCB
constexpr int     DASS_SPI_SCLK_PIN    = 12;       // GPIO — update per PCB
constexpr int     DASS_IMU_CS_PIN      = 10;       // GPIO — update per PCB
constexpr int     DASS_SPI_FREQ_HZ     = 10000000; // 10 MHz

// ---------------------------------------------------------------------------
// UART: Laser Range Finder  (JRT D09C)
// ---------------------------------------------------------------------------
constexpr int     DASS_LRF_UART_NUM    = 1;        // UART1
constexpr int     DASS_LRF_TX_PIN      = 17;       // GPIO — update per PCB
constexpr int     DASS_LRF_RX_PIN      = 18;       // GPIO — update per PCB
constexpr int     DASS_LRF_BAUD        = 19200;

// ---------------------------------------------------------------------------
// Display (390×390 AMOLED — SPI or MIPI-DSI, TBD per panel datasheet)
// ---------------------------------------------------------------------------
constexpr int     DASS_DISPLAY_WIDTH   = 390;
constexpr int     DASS_DISPLAY_HEIGHT  = 390;

// ---------------------------------------------------------------------------
// Camera (IMX477 via MIPI CSI-2)
// ---------------------------------------------------------------------------
// CSI lane/pin config deferred until camera driver bringup (Phase 4).

// ---------------------------------------------------------------------------
// Power / Range Trigger GPIO
// ---------------------------------------------------------------------------
constexpr int     DASS_RANGE_TRIGGER_PIN = 4;      // GPIO — update per PCB
constexpr int     DASS_POWER_BUTTON_PIN  = 0;      // GPIO — update per PCB
constexpr int     DASS_BATTERY_ADC_PIN   = 5;      // GPIO — update per PCB

// ---------------------------------------------------------------------------
// FreeRTOS Task Priorities  (higher = more important, max ~24 on ESP-IDF)
// ---------------------------------------------------------------------------
constexpr int     DASS_TASK_PRIO_SENSOR    = 10;   // Sensor poll loop
constexpr int     DASS_TASK_PRIO_BCE       = 9;    // DOPE engine update
constexpr int     DASS_TASK_PRIO_RENDER    = 8;    // Display / compositor
constexpr int     DASS_TASK_PRIO_UI        = 5;    // Settings menu / input

// ---------------------------------------------------------------------------
// FreeRTOS Task Stack Sizes (bytes)
// ---------------------------------------------------------------------------
constexpr int     DASS_STACK_SENSOR        = 4096;
constexpr int     DASS_STACK_BCE           = 8192;  // DOPE needs ~6 KB
constexpr int     DASS_STACK_RENDER        = 8192;
constexpr int     DASS_STACK_UI            = 4096;

// ---------------------------------------------------------------------------
// Timing / Rates
// ---------------------------------------------------------------------------
constexpr int     DASS_SENSOR_POLL_HZ      = 100;   // IMU/mag/baro poll rate
constexpr int     DASS_BCE_UPDATE_HZ       = 100;   // DOPE engine update rate
constexpr int     DASS_RENDER_FPS          = 60;    // Display refresh target
constexpr int     DASS_LRF_CONTINUOUS_HZ   = 4;     // Default continuous LRF rate (1–16 configurable)
