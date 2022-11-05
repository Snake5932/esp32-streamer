#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_psram.h"

#include "mqtt_client.h"

#include "module_info.h"
#include "camera.h"
#include "wifi_setup.h"

static const char *TAG = "main";

char camera_topic[128];
char camera_state_topic[128];
char camera_data_topic[128];
char camera_cmd_topic[128];

char *online_str = "online";
char *offline_str = "offline";

char *req_cmd = "req";
char *stop_cmd = "stop";
char *dump_cmd = "dump";

int dump_camera_subs = 0;
long long last_dump_msec = 0;

esp_mqtt_client_handle_t client;

static EventGroupHandle_t mqtt_event_group;
#define MOD_MQTT_HAS_SUBS_BIT BIT0


void show_chip_info()
{
	/* Print chip information */
	esp_chip_info_t chip_info;
	uint32_t flash_size;
	esp_chip_info(&chip_info);
	printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
		   CONFIG_IDF_TARGET,
		   chip_info.cores,
		   (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		   (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

	printf("silicon revision %d, ", chip_info.revision);
	if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
		printf("Get flash size failed");
		return;
	}

	printf("%uMB %s flash\n", flash_size / (1024 * 1024),
		   (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

}

static int is_cmd(char *topic, int topic_len, char *payload, int len, char *cmd) {
	return strncmp(topic, camera_cmd_topic, topic_len) == 0 && len == strlen(cmd) && strncmp(payload, cmd, len) == 0;
}

static void dump_camera() {
	printf("cam dump\n");
	char *buf;
	int size;
	mod_camera_fb_t fb = camera_get_fb(&buf, &size);
	if (fb == NULL) {
		ESP_LOGW(TAG, "cannot get camera buf handle\n");
		return;
	}
	ESP_LOGI(TAG, "publishing %d bytes\n", size);
	int ret = esp_mqtt_client_publish(client, camera_data_topic, buf, size, 0, 0);
	ESP_LOGI(TAG, "published ret: %d\n", ret);
	camera_free_fb(fb);
}

static void mqtt_event_handler(void *ptr, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			esp_mqtt_client_subscribe(client, camera_cmd_topic, 2);
			esp_mqtt_client_publish(client, camera_state_topic, online_str, 0, 2, 1);
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			break;
		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			printf("DATA=%.*s\r\n", event->data_len, event->data);
//			printf()
			if (is_cmd(event->topic, event->topic_len, event->data, event->data_len, req_cmd)) {
				dump_camera_subs++;
			} else if (is_cmd(event->topic, event->topic_len, event->data, event->data_len, stop_cmd)) {
				dump_camera_subs--;
				if (dump_camera_subs < 0) {
					dump_camera_subs = 0;
				}
			} else if (is_cmd(event->topic, event->topic_len, event->data, event->data_len, dump_cmd)) {
				dump_camera();
			}
			if (dump_camera_subs > 0) {
				printf("set bits\n");
				xEventGroupSetBits(mqtt_event_group, MOD_MQTT_HAS_SUBS_BIT);
			} else {
				printf("clear bits\n");
				xEventGroupClearBits(mqtt_event_group, MOD_MQTT_HAS_SUBS_BIT);
			}
			break;
		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
				ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
				ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
				ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
						 strerror(event->error_handle->esp_transport_sock_errno));
			} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
				ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
			} else {
				ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
			}
			break;
		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);
			break;
	}
}

void app_main(void)
{
	//needed for wi-fi module
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);
	show_chip_info();
    ret = wifi_init_sta();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "wifi connect error: %d\nrestarting\n", ret);
		esp_restart();
	}

	ret = camera_setup();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "setup camera err: %d\nrestarting\n", ret);
		esp_restart();
	}

	strcat(camera_topic, "cameras/");
	strcat(camera_topic, mod_camera_name);

	strcat(camera_state_topic, camera_topic);
	strcat(camera_state_topic, "/state");

	strcat(camera_data_topic, camera_topic);
	strcat(camera_data_topic, "/data");

	strcat(camera_cmd_topic, camera_topic);
	strcat(camera_cmd_topic, "/cmd");

	esp_mqtt_client_config_t mqtt_cfg = {
			.broker = {
					.address = {
							.hostname = mod_broker_ip,
							.port = mod_broker_port,
							.transport = MQTT_TRANSPORT_OVER_TCP
					}
			},
			.credentials = {
					.client_id = mod_camera_name,
					.username = mod_broker_username,
					.authentication = {
							.password = mod_broker_password
					}
			},
			.session = {
					.last_will = {
							.topic = camera_state_topic,
							.msg = offline_str,
							.msg_len = 0,
							.qos = 2,
							.retain = 1
					},
					.keepalive = 15
			},
			.network = {
					.reconnect_timeout_ms = mod_broker_reconnect_timeout_msec
			}
	};

#ifdef WITH_TLS
	mqtt_cfg.broker.verification.certificate = (const char*)mod_root_ca;
	mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_SSL;
	printf("root\n%s\n", (char*)mod_root_ca);
#endif
	printf("connection to mqtt host %s:%d\n", mod_broker_ip, mod_broker_port);
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);

	printf("ram left: %d, psram: %d\n", esp_get_free_heap_size(), esp_psram_get_size());

	TickType_t cam_ticks_per_frame = mod_dump_one_frame_msec / portTICK_PERIOD_MS;
	mqtt_event_group = xEventGroupCreate();

	while (1) {
		EventBits_t bits = xEventGroupWaitBits(mqtt_event_group, MOD_MQTT_HAS_SUBS_BIT,
							pdFALSE,
							pdFALSE,
							portMAX_DELAY);
		//TODO: check for error bit and break
		if (!(bits & MOD_MQTT_HAS_SUBS_BIT)) {
			ESP_LOGE(TAG, "wait bits returned %d\n", bits);
		} else {
			dump_camera();
			vTaskDelay(cam_ticks_per_frame);
		}
	}
//    for (int i = 10; i >= 0; i--) {
//        printf("Restarting in %d seconds...\n", i);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//    }
//    printf("Restarting now.\n");
//    fflush(stdout);
//    esp_restart();
}

