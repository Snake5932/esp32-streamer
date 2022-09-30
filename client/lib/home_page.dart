import 'package:flutter/material.dart';
import 'package:untitled/translation_page.dart';

import 'login_page.dart';

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {

  void navigateToTranslation() {
    Navigator.pushReplacement(context,
        MaterialPageRoute(builder:
            (context) => const TranslationPage())
    );
  }

  TextButton getButtonExit() {
    return TextButton(
      style: TextButton.styleFrom(
        foregroundColor: Colors.red,
      ),
      onPressed: () {
        Navigator.pushReplacement(context,
            MaterialPageRoute(builder:
                (context) => const LoginPage())
        );
      },
      child: const Text(
          'Выйти',
          style: TextStyle(
              fontSize: 24
          )
      ),
    );
  }

  TextButton getButtonListCameras() {
    return TextButton(
      style: TextButton.styleFrom(
        foregroundColor: Colors.green,
      ),
      onPressed: () {},
      child: const Text(
          'Список камер',
          style: TextStyle(
              fontSize: 24
          )
      ),
    );
  }

  TextButton getButtonCameraName(String name) {
    return TextButton(
      style: TextButton.styleFrom(
        foregroundColor: Colors.green,
      ),
      onPressed: () {
        Navigator.pushReplacement(context,
            MaterialPageRoute(builder:
                (context) => const TranslationPage())
        );
      },
      child: Text(
          name,
          style: const TextStyle(
          fontSize: 20
        )
      ),
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
        ),
      ),
      body: Column(
        children: [
          Center(
            child: getButtonListCameras()
          ),
          Row(
            children: [
              SizedBox(
                width: 50.0,
                height: 50.0,
                child: Image.asset(
                    'assets/images/webcam.png',
                    width: 50.0,
                    height: 50.0,
                    fit: BoxFit.fill
                ),
              ),
              Align(
                  alignment: Alignment.center,
                  child: getButtonCameraName('Some name')
              )
            ],
          ),
          Row(
            children: [
              SizedBox(
                width: 50.0,
                height: 50.0,
                child: Image.asset(
                    'assets/images/webcam.png',
                    width: 50.0,
                    height: 50.0,
                    fit: BoxFit.fill
                ),
              ),
              getButtonCameraName('Some name')
            ],
          ),
          Row(
            children: [
              SizedBox(
                width: 50.0,
                height: 50.0,
                child: Image.asset(
                    'assets/images/webcam.png',
                    width: 50.0,
                    height: 50.0,
                    fit: BoxFit.fill
                ),
              ),
              getButtonCameraName('Some name')
            ],
          )
        ],
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.centerFloat,
      floatingActionButton: getButtonExit()
    );
  }
}