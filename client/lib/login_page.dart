import 'package:flutter/material.dart';

import 'custom_form.dart';
import 'home_page.dart';
import 'mosquitto_manager.dart';

class LoginPage extends StatefulWidget {
  const LoginPage({Key? key}) : super(key: key);

  @override
  State<LoginPage> createState() {
    return _LoginPageState();
  }
}

class _LoginPageState extends State<LoginPage> {

  final _formKey = GlobalKey<FormState>();
  bool showErrorConnect = false;
  String server = '';
  String login = '';
  String password = '';

  @override
  void initState() {
    super.initState();
    showErrorConnect = false;
    server = '';
    login = '';
    password = '';
  }

  void _connect() async{
    _formKey.currentState?.save();
    final int rtServ = checkServer(server);
    if (rtServ == -1) {
      setState(() {
        showErrorConnect = true;
      });
      return;
    }
    final MQTTClientManager client = MQTTClientManager(server, login, password);
    final int rt = await client.connect();
    if (rt != 0) {
      setState(() {
        showErrorConnect = true;
      });
    } else {
      if (_formKey.currentState!.validate()) {
        if (mounted) {
          Navigator.of(context).push(
            MaterialPageRoute(
              builder: (_) => MyHomePage(manager: client),
            ),
          );
        }
      }
    }
  }

  int checkServer(String val) {
    List check = val.split(':');
    return check.length == 1 && check[0] != '' ? -1 : 0;
  }

  void setServer(String? val) {
    server = val!;
  }

  void setLogin(String? val) {
    login = val!;
  }

  void setPassword(String? val) {
    password = val!;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: Form(
          key: _formKey,
          child: Column(
            children: [
              CustomFormField(
                hintText: 'Сервер',
                onSubmit: setServer,
              ),
              CustomFormField(
                  hintText: 'Логин',
                  onSubmit: setLogin,
              ),
              CustomFormField(
                  hintText: 'Пароль',
                  onSubmit: setPassword,
              ),
              ElevatedButton(
                onPressed: _connect,
                child: const Text('Подтвердить'),
              ),
              if (showErrorConnect) const Text('Не удалось подключиться к серверу')
            ],
          )
        ),
      )
    );
  }

}