#ifndef ESP32_CAMERA_H
#define ESP32_CAMERA_H
#include "esp_err.h"

typedef void* mod_camera_fb_t;

esp_err_t camera_setup();
mod_camera_fb_t camera_get_fb(char **buf, int *size);
void camera_free_fb(mod_camera_fb_t fb);
#endif //ESP32_CAMERA_H
