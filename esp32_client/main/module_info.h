#ifndef ESP32_MODULE_H
#define ESP32_MODULE_H

extern const char *mod_wifi_ssid;
extern const char *mod_wifi_pass;

extern const char *mod_broker_ip;
extern const int mod_broker_port;

#ifdef WITH_TLS
extern const uint8_t mod_root_ca[] asm("_binary_ca_pem_start");
extern const uint8_t mod_root_ca_end[] asm("_binary_ca_pem_end");
#endif

extern const char *mod_broker_username;
extern const char *mod_broker_password;

extern const unsigned int mod_wifi_reconnect_timeout_msec;
extern const unsigned int mod_wifi_max_retry;

extern const unsigned int mod_broker_reconnect_timeout_msec;

extern const unsigned long mod_dump_one_frame_msec;

extern const char *mod_camera_name;
#endif //ESP32_MODULE_H