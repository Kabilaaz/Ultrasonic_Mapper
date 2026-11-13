idf_component_register(SRCS "main.c" "web_server.c"
                    INCLUDE_DIRS "."
	    	    REQUIRES esp_driver_gpio esp_wifi nvs_flash esp_timer esp_event esp_netif esp_http_server)
