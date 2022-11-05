# ESP32 Module

## Build
Build requires ESP-IDF build system installed.

```
cmake -DWITH_TLS=ON -B build
cd build
make -j4
make flash #flash binary to ESP32 to default USB device
make monitor #monitor serial data
```

`WITH_TLS` is optional macro and is disabled by default. It instructs module to use TLS connection to MQTT broker.
If present, user must also specify `ca.pem` file in `main` folder.
