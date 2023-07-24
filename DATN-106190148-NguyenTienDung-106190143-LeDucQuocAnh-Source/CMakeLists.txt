
#
# Main component of TF Micro project 'person_detection'.
#

idf_component_register(
    SRCS
        "recognition_responder.cc"
        "image_provider.cc"
        "main.cc"
        "main_functions.cc"
        "model_settings.cc"
        "number_recognition_model_data.cc"
        "app_camera_esp.c"
        "esp_lcd.c"
        "esp_cli.c"

    PRIV_REQUIRES console tflite-lib esp32-camera screen static_images spi_flash fb_gfx nvs_flash fatfs sdmmc freertos
    INCLUDE_DIRS "")
