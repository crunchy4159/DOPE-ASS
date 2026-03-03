/**
 * @file main.cpp
 * @brief DOPE-ASS firmware entry point for ESP32-P4.
 *
 * Initialises hardware buses, the DOPE ballistic engine, and launches
 * the primary FreeRTOS tasks (sensor polling, BCE update, rendering).
 *
 * Phase 0: skeleton that boots, inits DOPE, and prints a heartbeat.
 */

#include "dope_ass_config.h"
#include "dope/dope_api.h"
#include "dope/dope_types.h"

#ifdef DOPE_ASS_PLATFORM_ESP32P4
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "DOPE-ASS";
#endif

// ---------------------------------------------------------------------------
// Forward declarations — will move to dedicated files in later phases
// ---------------------------------------------------------------------------
static void task_bce_loop(void* arg);

// ---------------------------------------------------------------------------
// app_main  (ESP-IDF entry point)
// ---------------------------------------------------------------------------
#ifdef DOPE_ASS_PLATFORM_ESP32P4

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "DOPE-ASS v%d.%d  —  booting on ESP32-P4",
             DOPE_ASS_VERSION_MAJOR, DOPE_ASS_VERSION_MINOR);

    // ---- Phase 0: Initialise DOPE engine ----
    BCE_Init();
    ESP_LOGI(TAG, "DOPE engine initialised (BCE v%d.%d)",
             BCE_VERSION_MAJOR, BCE_VERSION_MINOR);

    // ---- Phase 1 (future): Init buses (I2C, SPI, UART) ----
    // hal::i2c_init();
    // hal::spi_init();
    // hal::uart_init();

    // ---- Create primary task ----
    xTaskCreatePinnedToCore(
        task_bce_loop,
        "bce_loop",
        DASS_STACK_BCE,
        nullptr,
        DASS_TASK_PRIO_BCE,
        nullptr,
        1  // Pin to core 1 (core 0 handles WiFi/BT if ever enabled)
    );

    ESP_LOGI(TAG, "Startup complete — entering idle.");
}

#else  // Native build (for compile-checking only)

int main()
{
    BCE_Init();
    return 0;
}

#endif // DOPE_ASS_PLATFORM_ESP32P4

// ---------------------------------------------------------------------------
// BCE Loop Task  —  Phase 0 heartbeat; Phase 1 will add sensor polling
// ---------------------------------------------------------------------------
static void task_bce_loop(void* /*arg*/)
{
#ifdef DOPE_ASS_PLATFORM_ESP32P4
    const TickType_t period = pdMS_TO_TICKS(1000 / DASS_BCE_UPDATE_HZ);

    while (true)
    {
        // Phase 1 will:
        //   1. Poll sensors → build SensorFrame
        //   2. BCE_Update(&frame)
        //   3. Read solution / faults
        //   4. Print or hand off to render task

        BCE_Mode mode = BCE_GetMode();
        ESP_LOGD(TAG, "BCE mode: %d", static_cast<int>(mode));

        vTaskDelay(period);
    }
#endif
}
