#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "web_server.h"

// Pulling in your processed arrays from main.c
extern volatile uint16_t us1Process[50];
extern volatile uint16_t us2Process[50];
extern volatile uint16_t us3Process[50];
extern volatile uint16_t us4Process[50];
extern volatile uint16_t us5Process[50];
extern volatile uint16_t us6Process[50];

static const char *TAG = "WEB_SERVER";

// --- HTTP GET Handler for the Data ---
static esp_err_t data_get_handler(httpd_req_t *req) {
    // Allocate a buffer large enough for 6 arrays of 50 integers + JSON formatting
    // (50 ints * ~4 chars) * 6 arrays + overhead ≈ 1500 bytes. 2048 is safe.
    char *json_buf = malloc(2048); 
    if (json_buf == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int offset = 0;
    offset += snprintf(json_buf + offset, 2048 - offset, "{\"us1\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us1Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "],\"us2\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us2Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "],\"us3\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us3Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "],\"us4\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us4Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "],\"us5\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us5Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "],\"us6\":[");
    for (int i = 0; i < 50; i++) offset += snprintf(json_buf + offset, 2048 - offset, "%d%s", us6Process[i], (i == 49) ? "" : ",");
    
    offset += snprintf(json_buf + offset, 2048 - offset, "]}");

    httpd_resp_set_type(req, "application/json");
    // Enable CORS so your external website can fetch this data without security errors
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); 
    httpd_resp_send(req, json_buf, HTTPD_RESP_USE_STRLEN);
    
    free(json_buf);
    return ESP_OK;
}

static const httpd_uri_t data_uri = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    .user_ctx  = NULL
};

// --- Wi-Fi SoftAP and Server Initialization ---
void init_web_server(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "MapperBot",
            .ssid_len = strlen("MapperBot"),
            .channel = 1,
            .password = "12345678", // Must be at least 8 characters
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP started. Connect to SSID: MapperBot, Pass: 12345678");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &data_uri);
        ESP_LOGI(TAG, "Web server started on http://192.168.4.1");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }
}