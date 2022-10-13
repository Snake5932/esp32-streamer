import 'dart:async';
import 'dart:io';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import 'camera.dart';
import 'config.dart';

class MQTTClientManager {
  late String broker;
  late String login;
  late String password;
  late MqttServerClient client;

  List<Camera> cameras = [];

  late StreamSubscription subscription;

  MQTTClientManager(String brok, String log, String pass) {
    int port;
    if (brok == '') {
      broker = Config.broker;
      port = Config.client_camera_port;
    } else {
      List check = brok.split(':');
      broker = check[0];
      port = int.parse(check[1]);
    }
    login = log == '' ? Config.test_login : log;
    password = pass == '' ? Config.test_password : pass;
    client = MqttServerClient.withPort(
        broker, Config.client_id, port);
  }

  Future<int> connect() async {
    client.logging(on: true);
    client.keepAlivePeriod = 60;
    client.onConnected = onConnected;
    client.onDisconnected = onDisconnected;
    client.onSubscribed = onSubscribed;
    client.pongCallback = pong;

    // client.setProtocolV311();
    // client.secure = true;
    // ByteData data = await rootBundle.load(Config.pathToCert);
    // SecurityContext context = SecurityContext();
    // context.setTrustedCertificatesBytes(data.buffer.asUint8List());
    // client.onBadCertificate = (Object a) => true;
    final connMessage = MqttConnectMessage()
        .withClientIdentifier(Config.client_id)
        .startClean() // Non persistent session for testing
        .withWillQos(MqttQos.atLeastOnce);
    client.connectionMessage = connMessage;

    try {
      await client.connect(login, password);
    } on NoConnectionException catch (e) {
      print('MQTTClient::Client exception - $e');
      client.disconnect();
      return -1;
    } on SocketException catch (e) {
      print('MQTTClient::Socket exception - $e');
      client.disconnect();
      return -1;
    } catch(e) {
      print(e);
      client.disconnect();
      return -1;
    }

    return 0;
  }

  void disconnect() {
    client.disconnect();
  }

  void subscribe(String topic) {
    client.subscribe(topic, MqttQos.atLeastOnce);
  }

  void onConnected() {
    print('MQTTClient::Connected');
  }

  void onDisconnected() {
    print('MQTTClient::Disconnected');
  }

  void onSubscribed(String topic) {
    print('MQTTClient::Subscribed to topic: $topic');
  }

  void pong() {
    print('MQTTClient::Ping response received');
  }

  void publishMessage(String topic, String message) {
    final builder = MqttClientPayloadBuilder();
    builder.addString(message);
    client.publishMessage(topic, MqttQos.exactlyOnce, builder.payload!);
  }

  Stream<List<MqttReceivedMessage<MqttMessage>>>? getMessagesStream() {
    return client.updates;
  }

  void _onMessage(List<MqttReceivedMessage> event) {
    print(event.length);
    final MqttPublishMessage recMess =
    event[0].payload as MqttPublishMessage;
    final String message =
    MqttPublishPayload.bytesToStringAsString(recMess.payload.message);

    print('[MQTT client] MQTT message: topic is <${event[0].topic}>, '
        'payload is <-- ${message} -->');
    print("[MQTT client] message with topic: ${event[0].topic}");
    print("[MQTT client] message with message: ${message}");
  }

}
