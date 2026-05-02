# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client esp_lcd driver)
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs protocol_examples_common)
endif()

# Add web server dependencies when enabled
if(CONFIG_ENABLE_WEB_SERVER)
    list(APPEND requires esp_http_server esp_littlefs)
endif()

idf_component_register(SRCS "esp_http_client_example.c"
	                    "font8x16.cc"
	                    "oled_display.cc"
                    INCLUDE_DIRS "."
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
