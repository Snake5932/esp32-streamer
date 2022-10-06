#include "esp_camera.h"
#include "esp_timer.h"
#include "WiFi.h"
#include "Arduino.h"
#include "PubSubClient.h"
#include "soc/soc.h"           //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

#include "module_info.h"
#include "esp32_setup.h"
#include "wifi_setup.h"

WiFiClient wifi_client;
PubSubClient client(server_ip, server_port, wifi_client);

String camera_topic = String("cameras/") + String(camera_name);
String camera_state_topic = camera_topic + String("/state");
String camera_data_topic = camera_topic + String("/data");
String camera_cmd_topic = camera_topic + String("/cmd");

unsigned long last_dump_msec = 0;
//use qos2 to send exactly 1 stream request to camera
int dump_camera_subscribers = 0;

//https://github.com/knolleary/pubsubclient/issues/791#issuecomment-800096719
void publish_large_mqtt(char* channel, uint8_t *data, uint32_t len) {
  unsigned long start_ts = millis();

  client.beginPublish(channel, len, false);

  size_t res;
  uint32_t offset = 0;
  uint32_t to_write = len;
  uint32_t buf_len;
  do {
    buf_len = to_write;
    if (buf_len > 64000)
      buf_len = 64000;

    res = client.write(data+offset, buf_len);

    offset += buf_len;
    to_write -= buf_len;
  } while (res == buf_len && to_write > 0);

  client.endPublish();

  Serial.printf("Published in MQTT channel %s: (binary data of length %d bytes, %d bytes written in %ld ms)\n", channel, len, len-to_write, millis()-start_ts);
}

int dump_camera() {
	camera_fb_t* fb = esp_camera_fb_get();
	Serial.printf("camera handle: %p\n", fb);
	if (fb == NULL) {
	  return -1;
	}
	Serial.println(fb->format);
	Serial.printf("%dx%d len: %d\n", fb->width, fb->height, fb->len);
	publish_large_mqtt((char*)camera_data_topic.c_str(), fb->buf, fb->len);
	esp_camera_fb_return(fb);

	return 0;
}

void print_bytes(unsigned char *buf, int len) {
  for (int i = 0; i < len; i++) {
    Serial.printf("%x ", buf[i]);
  }
  Serial.println();
}

bool is_cmd(char *topic, byte *payload, unsigned int len, char *cmd) {
	return strcmp(topic, camera_cmd_topic.c_str()) == 0 &&
		len == strlen(cmd) && strncmp(payload, cmd, len);
}

void callback(char* topic, byte* payload, unsigned int length) {
	int r;

	Serial.println(String("received msg in topic ") + String(topic) + " len " + String(length));
	print_bytes(payload, length);
	Serial.println();

  	if (is_cmd(topic, payload, length, "req")) {
    	dump_camera_subscribers++;
	} else if (is_cmd(topic, payload, length, "stop")) {
		dump_camera_subscribers--;
		if (dump_camera_subscribers < 0) {
			dump_camera_subscribers = 0;
		}
	} else if (is_cmd(topic, payload, length, "dump")) {
		dump_camera();
	}

  	// Serial.println(topic);
}

bool connect_to_server() {
	bool connected = client.connect(camera_name, username, password, camera_state_topic.c_str(), MQTTQOS1, true, "offline");
	if (connected) {
    	Serial.println("connected to server");
    	const char *ptr = camera_state_topic.c_str();
    	Serial.printf("ptr %p\n", ptr);
		client.subscribe(camera_cmd_topic.c_str());
    	client.publish(camera_state_topic.c_str(), "online", true);
	} else {
		Serial.printf("cannot connect to server\n");
	}

	Serial.printf("client state %d\n", client.state());
	return connected;
}

void setup() {
	int r;

	setup_esp32();
	//ssid and pass in module_info.ino
	setup_wifi(ssid, pass);

	client.setBufferSize(65535);
	client.setCallback(callback);

	connect_to_server();
}

void loop() {
	int r;

	if (!client.connected()) {
		r = client.state();
		Serial.printf("mqtt client state: %d\n", r);;
		connect_to_server();
	}

	client.loop();

	unsigned long curr_msec = millis();
	if (dump_camera_subscribers && curr_msec - last_dump_msec > dump_one_frame_msec) {
		last_dump_msec = curr_msec;
		dump_camera();
	}
	// delay(300);
}
