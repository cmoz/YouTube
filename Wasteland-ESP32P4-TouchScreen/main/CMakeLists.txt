FILE(GLOB_RECURSE main_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${main_srcs}
                        INCLUDE_DIRS "."
                        REQUIRES bsp_display
                                 bsp_i2c
                                 bsp_illuminate
                                 esp_timer)