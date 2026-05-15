idf_component_register(SRCS src/weather_api.c
                       INCLUDE_DIRS include
                       PRIV_REQUIRES esp_http_client json nvs_flash mbedtls)
