# Budget-G Pedal
100均素材 と AtomS3-Lite で錬成された、財布に優しすぎるゲーミングデバイス『Budget-G Pedal』

# Budget-G Pedal 制作手順書

M5Stack Atom S3 Lite を使用して、Wi-Fi経由で設定可能な「1ボタン・ショートカットキーボード」を作成する手順です。

## 1. 開発環境の準備
* **IDE:** VS Code (Visual Studio Code)
* **Extension:** PlatformIO IDE
* **Hardware:** M5Stack Atom S3 Lite, USB-Cケーブル

## 2. プロジェクトの作成
1. PlatformIOのホーム画面から `New Project` をクリック。
2. 以下の設定で作成:
   * **Name:** Budget-G_Pedal
   * **Board:** M5Stack AtomS3
   * **Framework:** Arduino

## 3. 設定ファイル (platformio.ini)
プロジェクト直下の `platformio.ini` を開き、内容をすべて以下に書き換えます。
※ USB設定 (`build_flags`) の等号 `=` の前後にスペースを入れないよう注意してください。

```ini
[env:m5stack-atoms3]
platform = espressif32
board = m5stack-atoms3
framework = arduino
monitor_speed = 115200

; 必要なライブラリ (M5Unified)
lib_deps = 
    m5stack/M5Unified @ ^0.1.16

; ビルド設定
; USB HID (キーボード) を有効にし、起動時のシリアル通信を確保する
build_flags = 
    -DARDUINO_USB_MODE=0
    -DARDUINO_USB_CDC_ON_BOOT=1
```

## 4. 書き込みと実行
Atom S3 LiteをPCに接続する。

PlatformIOの「Upload (矢印アイコン)」をクリックして書き込む。

重要: 書き込み後にエラーが出る、またはキーボードとして認識されない場合は、一度USBケーブルを抜き差ししてください。

## 5. 使い方
PC接続: USBケーブルでPCに接続（キーボードとして認識されます）。

スマホ接続: * Wi-Fi SSID: Budget-G_Pedal

Password: 12345678

設定: * 接続すると自動的に設定画面が開きます（開かない場合は http://192.168.4.1 へアクセス）。

モードやテキストを設定し「Update Pedal」を押す。

実行: 本体ボタンを押すと、設定したアクションがPCへ送信されます。

