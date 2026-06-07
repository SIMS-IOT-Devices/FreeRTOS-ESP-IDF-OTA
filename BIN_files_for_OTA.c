// First project, running from the factory partition, then switching to OTA_0 or OTA_1
// This is to test the OTA boot partition switching functionality.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include <string.h>

const esp_partition_t *running;

void app_main(void)
{
    // Get the currently running partition
    running = esp_ota_get_running_partition();
    printf("Starting Factory Project ...\n");
    printf("Running from: %s (offset 0x%08lx, subtype %d)\n", running->label, (unsigned long)running->address, (int)running->subtype);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Change to OTA_0 partition
    const esp_partition_t *next = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (next) {
        printf("Setting boot partition to: ota_0 (offset 0x%08lx)\n", (unsigned long)next->address);
        esp_err_t err = esp_ota_set_boot_partition(next);
        if (err == ESP_OK) {
            printf("Rebooting to boot from ota_0...\n");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            printf("esp_ota_set_boot_partition failed: %d\n", err);
        }
    } else {
        printf("OTA_0 partition not found\n");
    }

    // Change to OTA_1 partition
    // const esp_partition_t *next = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    // if (next) {
    //     printf("Setting boot partition to: ota_1 (offset 0x%08lx)\n", (unsigned long)next->address);
    //     esp_err_t err = esp_ota_set_boot_partition(next);
    //     if (err == ESP_OK) {
    //         printf("Rebooting to boot from ota_1...\n");
    //         vTaskDelay(2000 / portTICK_PERIOD_MS);
    //         esp_restart();
    //     } else {
    //         printf("esp_ota_set_boot_partition failed: %d\n", err);
    //     }
    // } else {
    //     printf("OTA_1 partition not found\n");
    // }
    
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
