import 'package:flutter/material.dart';

import 'home_page.dart';
import 'mosquitto_manager.dart';
// import 'package:untitled/main_page.dart';

class TranslationPage extends StatefulWidget {
  final MQTTClientManager? manager;
  const TranslationPage({super.key, required this.manager});

  @override
  State<TranslationPage> createState() {
    return _TranslationPageState();
  }
}

class _TranslationPageState extends State<TranslationPage> {

  Material getToBackButton(MQTTClientManager? manager) {
    return Material(
      color: Colors.transparent,
      child: MaterialButton(
          onPressed: () {
            Navigator.pushReplacement(context,
                MaterialPageRoute(builder:
                    (context) => MyHomePage(manager: manager))
            );
          },
        child: Image.asset('assets/images/green_arrow.png'),
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
        children: const [
          SizedBox(height: 10),
          Text(
            'Some name',
            style: TextStyle(
              fontSize: 20,
              color: Colors.green
            ),
          ),
          SizedBox(height: 30),
          Center(
            child: SizedBox(
              width: 400.0,
              height: 400.0,
              child: DecoratedBox(
                  decoration: BoxDecoration(
                      color: Colors.grey
                  )),
            ),
          ),
        ],
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.startFloat,
      floatingActionButton: getToBackButton(widget.manager),
    );
  }

}