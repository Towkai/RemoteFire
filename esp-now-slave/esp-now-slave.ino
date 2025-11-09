#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 8 // LED 連接在 GPIO 8 上
#define CHANNEL 1

#define BRIGHT 0
#define DARK 1

enum State {
    RECEIVING, // 接收中狀態
    MESSAGE_RECEIVED // 訊息接收狀態
};

State currentState = RECEIVING; // 初始狀態
unsigned long lastReceivedTime = 0; // 上次收到訊息的時間

// 函數：格式化 MAC 地址為字符串
String formatMacAddress(const uint8_t *mac_addr) {
    String macString = "";
    for (int i = 0; i < 6; i++) {
        macString += String(mac_addr[i], HEX);
        if (i < 5) macString += ":"; // 在每個字節之間添加冒號
    }
    return macString;
}

void onReceive(const esp_now_recv_info* info, const uint8_t *data, int len) {
    Serial.print("Received from: ");
    Serial.println(formatMacAddress(info->src_addr)); // 使用格式化函數
    Serial.print("Message: ");
    
    // 確保只顯示有效的字元數
    for (int i = 0; i < len; i++) {
        Serial.print((char)data[i]);
    }
    Serial.println(); // 換行

    currentState = MESSAGE_RECEIVED; // 設置狀態為訊息接收
    lastReceivedTime = millis(); // 記錄收到訊息的時間
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    WiFi.mode(WIFI_STA);
    Serial.println("ESP-NOW Receiver on CHANNEL " + String(CHANNEL));

    // 印出自身的 MAC 地址
    Serial.print("Slave MAC Address: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onReceive); // 註冊接收回調函數
}

void loop() {
    unsigned long currentTime = millis();

    switch (currentState) {
        case MESSAGE_RECEIVED:
            // 收到訊息時，LED 恆亮
            digitalWrite(LED_PIN, BRIGHT);

            // 如果兩秒內沒有接收到其他訊息，則回到接收中狀態
            if (currentTime - lastReceivedTime >= 2500) {
                currentState = RECEIVING; // 回到接收中狀態
            }
            break;

        case RECEIVING:
            // 接收中狀態，LED 燈閃爍
            digitalWrite(LED_PIN, BRIGHT);
            delay(100);
            digitalWrite(LED_PIN, DARK);
            delay(100);
            digitalWrite(LED_PIN, BRIGHT);
            delay(100);
            digitalWrite(LED_PIN, DARK);
            delay(700);
            break;
    }
}