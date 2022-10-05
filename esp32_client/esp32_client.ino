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

IPAddress server_ip(185, 188, 183, 121);
int server_port = 8080;

WiFiClient wifi_client;
PubSubClient client(server_ip, server_port, wifi_client);

String camera_topic = String("cameras/") + String(camera_name);
String camera_state_topic = camera_topic + String("/state");
String camera_data_topic = camera_topic + String("/data");

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

void print_bytes(char *buf) {
  int n = strlen(buf);
  for (int i = 0; i < n; i++) {
    Serial.printf("%x ", buf[i]);
  }
  Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  //Serial.println(String(payload));
  int r;
  if (strcmp(topic, "camera/dump") == 0) {
    camera_fb_t* fb = esp_camera_fb_get();
    Serial.printf("camera handle: %p\n", fb);
    if (fb == NULL) {
      return;
    }
    Serial.println(fb->format);
    Serial.printf("%dx%d len: %d\n", fb->width, fb->height, fb->len);
    publish_large_mqtt((char*)camera_data_topic.c_str(), fb->buf, fb->len);
    // r = client.beginPublish(camera_data_topic.c_str(), 128, false);
    // Serial.println(r);
    // r = client.write(fb->buf, 128);
    // Serial.println(r);
    // //client.write(fb->buf+60000, fb->len-60000);
    // r = client.endPublish();
    // Serial.println(r);
    esp_camera_fb_return(fb);
  }

  // Serial.println(topic);
  print_bytes(topic);
  Serial.println(String("received msg in topic ") + String(topic) + " len " + String(length));
}

bool connect_to_server() {
  bool connected = client.connect(camera_name, camera_state_topic.c_str(), MQTTQOS1, true, "offline");
  if (connected) {
    Serial.println("connected to server");
    const char *ptr = camera_state_topic.c_str();
    Serial.printf("ptr %p\n", ptr);
    client.publish(camera_state_topic.c_str(), "online", true);
    client.subscribe("inTopic");
    client.subscribe("camera/dump");
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
  if (!client.connected()) {
    connect_to_server();
  }
  client.loop();
  delay(300);
}
