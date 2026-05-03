set(requires esp_event nvs_flash esp_wifi esp_http_server dns_server mdns lwip esp_lcd driver json esp_timer)

idf_build_get_property(target IDF_TARGET)

# Web server and filesystem dependencies
if(CONFIG_ENABLE_WEB_SERVER)
    list(APPEND requires esp_littlefs)
endif()

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
    set(WEB_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web/dist")

    if(EXISTS ${WEB_DIST_DIR})
        littlefs_create_partition_image(
            www ${WEB_DIST_DIR}
            FLASH_IN_PROJECT
        )
        message(STATUS "Web frontend found at ${WEB_DIST_DIR}, embedding into www partition")
    else()
        message(WARNING
            "Web frontend not built. "
            "Run 'cd web && pnpm install && pnpm build' first, "
            "or disable CONFIG_ENABLE_WEB_SERVER in menuconfig."
        )
    endif()
endif()
