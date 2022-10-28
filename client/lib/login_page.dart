import 'package:flutter/material.dart';

import 'config.dart';
import 'custom_form.dart';
import 'home_page.dart';
import 'mosquitto_manager.dart';

class LoginPage extends StatefulWidget {
  final MQTTClientManager? manager;

  const LoginPage({super.key, required this.manager});

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

  void _connect() async {
    _formKey.currentState?.save();
    final int rtServ = checkServer(server);
    if (rtServ == -1) {
      setState(() {
        showErrorConnect = true;
      });
      return;
    }
    widget.manager?.setData(server, login, password);
    final int? rt = await widget.manager?.connect();
    if (rt == null || rt != 0) {
      setState(() {
        showErrorConnect = true;
      });
    } else {
      if (_formKey.currentState!.validate()) {
        if (mounted) {
          widget.manager?.subscribe(Config.topics['ALL_CAMERAS']!);
          Navigator.of(context).push(
            MaterialPageRoute(
              builder: (_) => MyHomePage(manager: widget.manager),
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
      body: Form(
         key: _formKey,
         child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
             crossAxisAlignment: CrossAxisAlignment.center,
             children: [
               CustomFormField(
                 decoration: const InputDecoration(
                     hintText: 'Адрес сервера',
                     hintStyle: TextStyle(color: Colors.green),
                     enabledBorder: OutlineInputBorder(
                         borderSide: BorderSide(color: Colors.green))),
                 onSubmit: setServer,
               ),
               CustomFormField(
                 decoration: const InputDecoration(
                     hintText: 'Логин',
                     hintStyle: TextStyle(color: Colors.green),
                     enabledBorder: OutlineInputBorder(
                         borderSide: BorderSide(color: Colors.green))),
                 onSubmit: setLogin,
               ),
               CustomFormField(
                 decoration: const InputDecoration(
                   hintText: 'Пароль',
                   hintStyle: TextStyle(color: Colors.green),
                   enabledBorder: OutlineInputBorder(
                       borderSide: BorderSide(color: Colors.green)),
                 ),
                 onSubmit: setPassword,
               ),
               if (showErrorConnect) const Text('Не удалось подключиться к серверу')
             ]),
       ),
       floatingActionButton: ElevatedButton(
           style: ButtonStyle(
               backgroundColor: MaterialStateProperty.all(Colors.white),
               minimumSize:
                   MaterialStateProperty.all(const Size.fromHeight(52)),
               side: MaterialStateProperty.all(
                   const BorderSide(color: Colors.green, width: 5))),
           onPressed: _connect,
           child: const Text(
             'Подтвердить',
             style: TextStyle(color: Colors.green, fontSize: 20),
           ),
         ),
       floatingActionButtonLocation: FloatingActionButtonLocation.centerFloat,
    );
  }
}
