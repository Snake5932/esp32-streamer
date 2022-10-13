#include "esp_camera.h"
#include "esp_timer.h"
#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "soc/soc.h"           //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

#include "sys/poll.h"
#include "sys/socket.h"

// #if WITH_TLS == 1
// #include "WiFiClientSecure.h"
// #endif

#include "WiFiClientSecure.h"

#include "module_info.h"
#include "esp32_setup.h"
#include "wifi_setup.h"

#if WITH_TLS == 1
WiFiClientSecure wifi_client;
// wifi_client.setCACert(tls_cert);
#else
WiFiClient wifi_client;
#endif

PubSubClient client(server_ip, server_port, wifi_client);

String camera_topic = String("cameras/") + String(camera_name);
String camera_state_topic = camera_topic + String("/state");
String camera_data_topic = camera_topic + String("/data");
String camera_cmd_topic = camera_topic + String("/cmd");

unsigned long last_dump_msec = 0;
unsigned long last_reconnect_try_msec = 0;
//use qos2 to send exactly 1 stream request to camera
int dump_camera_subscribers = 0;

//https://github.com/knolleary/pubsubclient/issues/791#issuecomment-800096719
//change buildheader method argument to uint32_t and corresponding local variable
void publish_large_mqtt(char* channel, uint8_t *data, uint32_t len) {
  unsigned long start_ts = millis();

  client.beginPublish(channel, len, false);

  int res;
  uint32_t offset = 0;
  uint32_t to_write = len;
  
  int socket = wifi_client.fd();
  struct pollfd pfds;
  pfds.fd = socket;
  pfds.events = POLLOUT;

  do {
    Serial.printf("heap size left: %d, wifi_client: %d\n", ESP.getFreeHeap(), wifi_client.availableForWrite());
    res = poll(&pfds, 1, -1);
    Serial.printf("poll ret: %d %d %d %d\n", res, pfds.revents, errno, res < 0);
    if (res < 0) {
      Serial.printf("poll err %d\n", errno);
      break;
    }
    if ((pfds.revents & POLLERR) || (pfds.revents & POLLHUP) ||
        (pfds.revents & POLLNVAL)) {
      break;
    }
    res = send(socket, data+offset, to_write, 0);
    // res = client.write(data+offset, to_write);
    Serial.printf("written %d %d\n", res, errno);
    if (res < 0) {
      break;
    }

    offset += res;
    to_write -= res;
  } while (to_write > 0);

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
	int part = fb->len/3;
  Serial.printf("heap size left: %d\n", ESP.getFreeHeap());
	publish_large_mqtt((char*)camera_data_topic.c_str(), fb->buf, fb->len);
  // publish_large_mqtt((char*)camera_data_topic.c_str(), fb->buf + part, part);
  // publish_large_mqtt((char*)camera_data_topic.c_str(), fb->buf + 2*part, part);
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
		len == strlen(cmd) && strncmp((const char*)payload, cmd, len) == 0;
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
#if WITH_TLS == 1
	wifi_client.setCACert(tls_cert);
#endif
	client.setBufferSize(65535);
	client.setCallback(callback);

	connect_to_server();
}

void loop() {
	int r;

	unsigned long curr_msec = millis();

	if (!client.connected() && curr_msec - last_reconnect_try_msec > reconnect_timeout_msec) {
    	last_reconnect_try_msec = curr_msec;

		r = client.state();
		Serial.printf("mqtt client state: %d\n", r);;

		connect_to_server();

#if WITH_TLS == 1
		char buf[256];
		wifi_client.lastError(buf, 256);
		Serial.printf("tls err: %s\n", buf);
#endif
	}

	client.loop();

	if (dump_camera_subscribers && curr_msec - last_dump_msec > dump_one_frame_msec) {
		last_dump_msec = curr_msec;
		dump_camera();
	}
	// delay(300);
}
