#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <LittleFS.h>

// --- Wi-Fi設定 ---
const char *ssid = "Budget-G_Pedal";
const char *password = "12345678";
const char *CONFIG_FILE = "/config.txt";
// ----------------

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
USBHIDKeyboard Keyboard;
WebServer server(80);

// 状態管理
String textBuffer = "Hello Budget-G";
// 0:Text, 1:Copy, 2:Paste, 3:Lock, 4:Enter, 5:Tab, 6:Space, 7:SingleKey
int currentMode = 0; 

// --- 設定保存・読み込み関数 ---
void saveSettings() {
  File file = LittleFS.open(CONFIG_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println(currentMode);
  file.print(textBuffer);
  file.close();
  Serial.println("Saved: Mode=" + String(currentMode));
}

void loadSettings() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  if (LittleFS.exists(CONFIG_FILE)) {
    File file = LittleFS.open(CONFIG_FILE, FILE_READ);
    if (file) {
      String modeStr = file.readStringUntil('\n');
      currentMode = modeStr.toInt();
      if (file.available()) {
        textBuffer = file.readString();
      }
      file.close();
      Serial.println("Loaded: Mode=" + String(currentMode));
    }
  }
}

// --- Web UI ---
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
      var inputLabel = document.getElementById("inputLabel");
      
      // Text Mode(0) または Single Key(7) の場合は入力欄を表示
      if(mode == "0") { 
        inputField.style.display = "block";
        inputLabel.innerText = "Text to Type:";
      } 
      else if (mode == "7") {
        inputField.style.display = "block";
        inputLabel.innerText = "Single Key (Char):";
      }
      else { 
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
        <option value="0" %SELECTED_0%>Text Input (String)</option>
        <option value="1" %SELECTED_1%>Ctrl + C (Copy)</option>
        <option value="2" %SELECTED_2%>Ctrl + V (Paste)</option>
        <option value="3" %SELECTED_3%>Win + L (Lock PC)</option>
        <option value="4" %SELECTED_4%>Enter Key</option>
        <option value="5" %SELECTED_5%>Tab Key</option>
        <option value="6" %SELECTED_6%>Space Key</option>
        <option value="7" %SELECTED_7%>Single Key (Press)</option>
      </select>
      
      <div id="textInput">
        <label id="inputLabel">Text Content:</label>
        <input type="text" name="msg" value="%CURRENT_TEXT%" placeholder="Enter text or char...">
      </div>
      
      <input type="submit" value="Update & Save">
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
    case 5: return "Action: Tab";
    case 6: return "Action: Space";
    case 7: return "Key Press: '" + textBuffer.substring(0, 1) + "'";
    default: return "Unknown";
  }
}

void handleRoot() {
  String html = index_html;
  html.replace("%CURRENT_TEXT%", textBuffer);
  html.replace("%CURRENT_STATUS%", getStatusString());
  for(int i=0; i<=7; i++){ // ループ回数を7まで拡張
    String key = "%SELECTED_" + String(i) + "%";
    if(i == currentMode) html.replace(key, "selected");
    else html.replace(key, "");
  }
  server.send(200, "text/html", html);
}

void handleNotFound() {
  String message = "Redirecting to Budget-G Pedal...";
  server.sendHeader("Location", String("http://") + apIP.toString(), true); 
  server.send(302, "text/plain", message);
}

void handleSet() {
  if (server.hasArg("mode")) currentMode = server.arg("mode").toInt();
  if (server.hasArg("msg")) textBuffer = server.arg("msg");
  
  saveSettings();

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

  loadSettings();

  Keyboard.begin();
  USB.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password);

  dnsServer.start(DNS_PORT, "*", apIP);
  
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.begin(115200);
  M5.Display.fillScreen(TFT_GREEN);
  delay(500);
  M5.Display.fillScreen(TFT_BLACK);
}

void loop() {
  M5.update();
  dnsServer.processNextRequest();
  server.handleClient();

  if (M5.BtnA.wasPressed()) {
    M5.Display.fillScreen(TFT_WHITE);
    switch (currentMode) {
      case 0: // Text Input
        if (textBuffer.length() > 0) Keyboard.print(textBuffer); 
        break;
      case 1: // Copy
        Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('c'); delay(50); Keyboard.releaseAll(); 
        break;
      case 2: // Paste
        Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('v'); delay(50); Keyboard.releaseAll(); 
        break;
      case 3: // Lock
        Keyboard.press(KEY_LEFT_GUI); Keyboard.press('l'); delay(50); Keyboard.releaseAll(); 
        break;
      case 4: // Enter
        Keyboard.write(KEY_RETURN); 
        break;
      case 5: // Tab
        Keyboard.write(KEY_TAB); 
        break;
      case 6: // Space
        Keyboard.write(' '); 
        break;
      case 7: // Single Key Press (新規追加)
        if (textBuffer.length() > 0) {
          // 入力された文字列の1文字目を取得
          char keyToPress = textBuffer.charAt(0);
          
          // Press -> Delay -> Release のパターンで確実に送信
          Keyboard.press(keyToPress);
          delay(50); // ゲーム等で認識させるために少し待つ
          Keyboard.releaseAll();
        }
        break;
    }
    delay(100);
    M5.Display.fillScreen(TFT_BLACK);
  }
  delay(10);
}