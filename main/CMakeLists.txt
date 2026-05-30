set(requires esp_event nvs_flash esp_wifi esp_http_server dns_server mdns lwip esp_lcd driver json esp_timer)

idf_build_get_property(target IDF_TARGET)

# Web server and filesystem dependencies
if(CONFIG_ENABLE_WEB_SERVER)
    list(APPEND requires esp_littlefs)
endif()

set(requires ${requires} weather_api)

idf_component_register(SRCS "main.cc"
                            "font8x16.cc"
                            "oled_display.cc"
                            "web_server/src/web_server.cc"
                            "web_server/src/rest_api.cc"
                            "web_server/src/fs_handler.cc"
                    INCLUDE_DIRS "."
                                  "web_server/include"
                    PRIV_REQUIRES ${requires}
                    EMBED_TXTFILES howsmyssl_com_root_cert.pem)

# ====== Web frontend: embed LittleFS partition image into firmware ======
if(CONFIG_ENABLE_WEB_SERVER)
    set(WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web")

    if(EXISTS ${WEB_DIR})
        littlefs_create_partition_image(
            www ${WEB_DIR}
            FLASH_IN_PROJECT
        )
        message(STATUS "Web frontend found at ${WEB_DIR}, embedding into www partition")
    else()
        message(WARNING
            "Web frontend not found. "
            "Ensure web/index.html exists, "
            "or disable CONFIG_ENABLE_WEB_SERVER in menuconfig."
        )
    endif()
endif()
