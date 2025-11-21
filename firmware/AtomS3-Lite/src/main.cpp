#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>  // 追加: DNSサーバー用
#include "USB.h"
#include "USBHIDKeyboard.h"

// --- 設定エリア ---
const char *ssid = "Budget-G_Pedal";
const char *password = "12345678";
// ----------------

const byte DNS_PORT = 53; // DNSポート番号
IPAddress apIP(192, 168, 4, 1); // 固定IP設定
DNSServer dnsServer; // DNSサーバーインスタンス
USBHIDKeyboard Keyboard;
WebServer server(80);

// 状態管理
String textBuffer = "Hello Budget-G";
int currentMode = 0; 

// HTMLページ (変更なし)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>Budget-G Pedal</title>
  <style>
    body { font-family: sans-serif; text-align: center; padding: 20px; background-color: #f4f4f4; }
    .container { max-width: 400px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    select, input[type=text] { width: 100%; padding: 12px; font-size: 16px; margin-bottom: 15px; box-sizing: border-box; }
    input[type=submit] { background-color: #007BFF; color: white; padding: 12px; border: none; width: 100%; font-size: 16px; cursor: pointer; border-radius: 5px;}
    input[type=submit]:hover { background-color: #0056b3; }
    .status { margin-top: 20px; padding: 10px; background: #e9ecef; border-radius: 5px; }
    .footer { margin-top: 20px; font-size: 0.8em; color: #aaa; }
  </style>
  <script>
    function updateUI() {
      var mode = document.getElementById("modeSelect").value;
      var inputField = document.getElementById("textInput");
      if(mode == "0") {
        inputField.style.display = "block";
      } else {
        inputField.style.display = "none";
      }
    }
  </script>
</head>
<body onload="updateUI()">
  <div class="container">
    <h2>Budget-G Pedal</h2>
    <form action="/set" method="GET">
      <label>Pedal Action:</label>
      <select id="modeSelect" name="mode" onchange="updateUI()">
        <option value="0" %SELECTED_0%>Text Input</option>
        <option value="1" %SELECTED_1%>Ctrl + C (Copy)</option>
        <option value="2" %SELECTED_2%>Ctrl + V (Paste)</option>
        <option value="3" %SELECTED_3%>Win + L (Lock PC)</option>
        <option value="4" %SELECTED_4%>Enter Key</option>
      </select>
      <div id="textInput">
        <label>Text Content:</label>
        <input type="text" name="msg" value="%CURRENT_TEXT%" placeholder="Enter text...">
      </div>
      <input type="submit" value="Update Pedal">
    </form>
    <div class="status">
      <strong>Current Setting:</strong><br>
      <span style="color:#d9534f; font-weight:bold;">%CURRENT_STATUS%</span>
    </div>
    <div class="footer">Budget-G Pedal Configurator</div>
  </div>
</body>
</html>
)rawliteral";

String getStatusString() {
  switch(currentMode) {
    case 0: return "Type: \"" + textBuffer + "\"";
    case 1: return "Action: Ctrl + C";
    case 2: return "Action: Ctrl + V";
    case 3: return "Action: Win + L";
    case 4: return "Action: Enter";
    default: return "Unknown";
  }
}

void handleRoot() {
  String html = index_html;
  html.replace("%CURRENT_TEXT%", textBuffer);
  html.replace("%CURRENT_STATUS%", getStatusString());
  for(int i=0; i<=4; i++){
    String key = "%SELECTED_" + String(i) + "%";
    if(i == currentMode) html.replace(key, "selected");
    else html.replace(key, "");
  }
  server.send(200, "text/html", html);
}

// キャプティブポータル用: 未知のURLへのアクセスをルートに転送
void handleNotFound() {
  String message = "Redirecting to Budget-G Pedal...";
  server.sendHeader("Location", String("http://") + apIP.toString(), true); 
  server.send(302, "text/plain", message);
}

void handleSet() {
  if (server.hasArg("mode")) currentMode = server.arg("mode").toInt();
  if (server.hasArg("msg")) textBuffer = server.arg("msg");
  
  Serial.println("Updated - Mode: " + String(currentMode) + ", Text: " + textBuffer);

  // 設定後はルートに戻る
  server.sendHeader("Location", String("http://") + apIP.toString());
  server.send(303);
  
  M5.Display.fillScreen(TFT_BLUE);
  delay(200);
  M5.Display.fillScreen(TFT_BLACK);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(128);
  M5.Display.fillScreen(TFT_RED);

  Keyboard.begin();
  USB.begin();

  // Wi-Fi AP設定 (IPを明示的に指定)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  // DNSサーバー開始 (すべてのドメイン "*" を自分自身 apIP に解決させる)
  dnsServer.start(DNS_PORT, "*", apIP);
  
  Serial.begin(115200);
  Serial.print("Budget-G Pedal IP: ");
  Serial.println(apIP);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  
  // Android/iOSの接続チェック(generate_204など)をすべてキャッチする
  server.onNotFound(handleNotFound);
  
  server.begin();

  M5.Display.fillScreen(TFT_GREEN);
  delay(500);
  M5.Display.fillScreen(TFT_BLACK);
}

void loop() {
  M5.update();
  dnsServer.processNextRequest(); // DNSリクエストの処理 (必須)
  server.handleClient();

  if (M5.BtnA.wasPressed()) {
    M5.Display.fillScreen(TFT_WHITE);
    switch (currentMode) {
      case 0: if (textBuffer.length() > 0) Keyboard.print(textBuffer); break;
      case 1: Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('c'); delay(50); Keyboard.releaseAll(); break;
      case 2: Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('v'); delay(50); Keyboard.releaseAll(); break;
      case 3: Keyboard.press(KEY_LEFT_GUI); Keyboard.press('l'); delay(50); Keyboard.releaseAll(); break;
      case 4: Keyboard.write(KEY_RETURN); break;
    }
    delay(100);
    M5.Display.fillScreen(TFT_BLACK);
  }
  delay(10);
}