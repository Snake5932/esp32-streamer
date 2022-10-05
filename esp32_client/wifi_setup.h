String get_wifi_status(int status) {
    switch(status){
        case WL_IDLE_STATUS:
        return "WL_IDLE_STATUS";
        case WL_SCAN_COMPLETED:
        return "WL_SCAN_COMPLETED";
        case WL_NO_SSID_AVAIL:
        return "WL_NO_SSID_AVAIL";
        case WL_CONNECT_FAILED:
        return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST:
        return "WL_CONNECTION_LOST";
        case WL_CONNECTED:
        return "WL_CONNECTED";
        case WL_DISCONNECTED:
        return "WL_DISCONNECTED";
    }
}

int setup_wifi(const char *ssid, const char *key) {
  /*Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();
  Serial.print("Number of available WiFi networks discovered:");
  Serial.println(numSsid);
  */
  Serial.println("init step 4: try to connect to wifi "+String(ssid)+" with password "+String(key)+" ...");
  int status = WL_IDLE_STATUS;
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WEP network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, key);

    // wait 10 seconds for connection:
    delay(5000);
    status = WiFi.status();
    Serial.println(get_wifi_status(status));
  }
  Serial.println(get_wifi_status(WiFi.status()));
  Serial.print("You're connected to the network");
  //delay(5000);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  return 0;
}
