// Download a proj4.bin file via WiFi STA from the PC running a Python HTTP server 
// and flash it to the ESP32C3 using OTA update to ESP32C3 as a Client.
// The URL on the Server: http://192.168.1.35:8000/proj4.bin

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "my_data.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include <string.h>
#include "esp_http_client.h"
#include "esp_https_ota.h"

const esp_partition_t *running;
volatile bool wifi_ready = false;

void wifi_event_handler(void *event_handler_arg,
                        esp_event_base_t event_base,
                        int32_t event_id,
                        void *event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            printf("WiFi connecting WIFI_EVENT_STA_START ... \n");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            printf("WiFi connected WIFI_EVENT_STA_CONNECTED ... \n");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            printf("WiFi lost connection WIFI_EVENT_STA_DISCONNECTED. Retrying connection...");
            esp_wifi_connect();
            break;
        case IP_EVENT_STA_GOT_IP:
            printf("WiFi got IP ... \n\n");
            wifi_ready = true;
            break;
        default:
            break;
    }
}

void wifi_connection(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        printf("Erasing NVS flash...\n");
        esp_err_t erase_ret = nvs_flash_erase();
        if (erase_ret != ESP_OK) {
            printf("NVS flash erase failed: 0x%x\n", erase_ret);
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        printf("NVS Flash init failed: 0x%x\n", ret);
        return;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        printf("esp_netif_init failed: 0x%x\n", ret);
        return;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        printf("esp_event_loop_create_default failed: 0x%x\n", ret);
        return;
    }

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        printf("esp_netif_create_default_wifi_sta failed\n");
        return;
    }

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&wifi_initiation);
    if (ret != ESP_OK) {
        printf("esp_wifi_init failed: 0x%x\n", ret);
        return;
    }

    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        printf("esp_event_handler_register for WIFI_EVENT failed: 0x%x\n", ret);
        return;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        printf("esp_event_handler_register for IP_EVENT failed: 0x%x\n", ret);
        return;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        printf("esp_wifi_set_mode failed: 0x%x\n", ret);
        return;
    }

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS,
        },
    };

    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    if (ret != ESP_OK) {
        printf("esp_wifi_set_config failed: 0x%x\n", ret);
        return;
    }

    ret = esp_wifi_set_ps(WIFI_PS_NONE);
    if (ret != ESP_OK) {
        printf("esp_wifi_set_ps failed: 0x%x\n", ret);
        return;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        printf("esp_wifi_start failed: 0x%x\n", ret);
        return;
    }

    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        printf("esp_wifi_connect failed: 0x%x\n", ret);
        return;
    }

    printf("Waiting for WiFi connection...\n");
    while (!wifi_ready) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR\n");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            printf("HTTP_EVENT_ON_CONNECTED\n");
            break;
        case HTTP_EVENT_HEADER_SENT:
            printf("HTTP_EVENT_HEADER_SENT\n");
            break;
        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_ON_FINISH:
            printf("HTTP_EVENT_ON_FINISH\n");
            break;
        case HTTP_EVENT_DISCONNECTED:
            printf("HTTP_EVENT_DISCONNECTED\n");
            break;
        case HTTP_EVENT_REDIRECT:
            printf("HTTP_EVENT_REDIRECT\n");
            break;
    }
    return ESP_OK;
}

// Download and flash OTA image from HTTP server
void download_and_flash_ota(const char *url)
{
    printf("\n=== Starting OTA Download ===\n");
    printf("URL: %s\n", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .skip_cert_common_name_check = true,
    };
    
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        printf("\n=== OTA Image Flashed Successfully ===\n");
        printf("Device will restart to boot from new firmware...\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        printf("\n=== OTA Failed: 0x%x ===\n", ret);
    }
}

void app_main(void) { 
    wifi_connection();

    // Get the currently running partition
    running = esp_ota_get_running_partition();
    printf("Starting Factory Project ...\n");
    printf("Running from: %s (offset 0x%08lx, subtype %d)\n", running->label, (unsigned long)running->address, (int)running->subtype);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Download and flash OTA firmware from Python server
    printf("\nAttempting to download firmware from server...\n");
    download_and_flash_ota("http://192.168.1.35:8000/proj4.bin");

    while (1) {
        printf("WiFi is connected and ready to use ... \n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
