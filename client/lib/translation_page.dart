import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:image/image.dart' as img;
import 'package:mqtt_client/mqtt_client.dart';

import 'home_page.dart';
import 'mosquitto_manager.dart';

class TranslationPage extends StatefulWidget {
  final MQTTClientManager? manager;
  final String cameraName;

  const TranslationPage({super.key, required this.manager, required this.cameraName});

  @override
  State<TranslationPage> createState() {
    return _TranslationPageState();
  }
}

class _TranslationPageState extends State<TranslationPage> {

  @override
  void initState() {
    super.initState();
    subscribeToStream();
  }

  void subscribeToStream() {
    widget.manager?.subscribe('cameras/${widget.cameraName}/data');
    widget.manager?.publishMessage('cameras/${widget.cameraName}/cmd', 'req');
  }

  Material getToBackButton(MQTTClientManager? manager) {
    return Material(
      color: Colors.transparent,
      child: MaterialButton(
          onPressed: () {
            widget.manager?.publishMessage('cameras/${widget.cameraName}/cmd', 'stop');
            widget.manager?.client.unsubscribe('cameras/${widget.cameraName}/data');
            Navigator.pop(context,
                MaterialPageRoute(builder:
                    (context) => MyHomePage(manager: manager))
            );
          },
        child: Image.asset('assets/images/green_arrow.png'),
      )
    );
  }

  Widget bodyStream() {
    return Container(
      color: Colors.black,
      child: StreamBuilder(
        stream: widget.manager?.client.updates,
        builder: (context, snapshot) {
          if (!snapshot.hasData) {
            return const Center(
              child: CircularProgressIndicator(
                valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
              )
            );
          } else {
            final mqttRecievedMessages = snapshot.data;
            final recvMsg = mqttRecievedMessages![0].payload as MqttPublishMessage;
            img.Image? data;
            try {
              data = img.decodeJpg(recvMsg.payload.message);
            } catch (e) {
              return const Center(
                  child: CircularProgressIndicator(
                    valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                  )
              );
            }

            return Image.memory(img.encodeJpg(data!) as Uint8List, gaplessPlayback: true);
          }
        },

      )
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green,
        automaticallyImplyLeading: false,
        title: Center(
            widthFactor: 50,
            child: SizedBox(
                width: 50,
                child: Image.asset('assets/images/title.png')
            )
        )
      ),
      body: Column(
        children: [
          const SizedBox(height: 10),
          Text(
            widget.cameraName,
            style: const TextStyle(
              fontSize: 20,
              color: Colors.green
            ),
          ),
          const SizedBox(height: 30),
          Center(
            child: SizedBox(
              width: 400.0,
              height: 400.0,
              child: bodyStream(),
            ),
          ),
        ],
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.startFloat,
      floatingActionButton: getToBackButton(widget.manager),
    );
  }

}