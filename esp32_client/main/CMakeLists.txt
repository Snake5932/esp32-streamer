idf_component_register(SRCS "esp32_client.c"
							"wifi_setup.c"
							"module_info.c"
							"camera.c"
                    INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")

