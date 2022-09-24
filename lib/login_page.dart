import 'package:flutter/material.dart';
import 'package:untitled/home_page.dart';
// import 'package:untitled/main_page.dart';

import 'custom_form.dart';

class LoginPage extends StatefulWidget {
  const LoginPage({Key? key}) : super(key: key);

  @override
  State<LoginPage> createState() {
    return _LoginPageState();
  }
}

class _LoginPageState extends State<LoginPage> {

  final _formKey = GlobalKey<FormState>();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Form(
          key: _formKey,
          child: Column(
            children: [
              CustomFormField(
                hintText: 'Сервер'
              ),
              CustomFormField(
                  hintText: 'Логин'
              ),
              CustomFormField(
                  hintText: 'Пароль'
              ),
              ElevatedButton(
                onPressed: () {
                  if (_formKey.currentState!.validate()) {
                    Navigator.of(context).push(
                      MaterialPageRoute(
                        builder: (_) => const MyHomePage(title: "Flutter"),
                      ),
                    );
                  }
                },
                child: const Text('Подтвердить'),
              )
            ],
          )
        ),
      )
    );
  }

}