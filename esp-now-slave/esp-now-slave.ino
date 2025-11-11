#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 8 // LED 連接在 GPIO 8 上
#define RELAY_PIN 10 // 繼電器 連接在 GPIO 10 上
#define CHANNEL 1

#define BRIGHT 0
#define DARK 1

enum State {
    STANDBY, // 接收中狀態
    RECEIVED // 訊息接收狀態
};

State currentState = STANDBY; // 初始狀態
unsigned long lastReceivedTime = 0; // 上次收到訊息的時間
unsigned long lastFireTime = 0; // 上次收到訊息的時間
TaskHandle_t ledTaskHandle = NULL; // 任務句柄

// 函數：格式化 MAC 地址為字符串
String formatMacAddress(const uint8_t *mac_addr) {
    String macString = "";
    for (int i = 0; i < 6; i++) {
        macString += String(mac_addr[i], HEX);
        if (i < 5) macString += ":"; // 在每個字節之間添加冒號
    }
    return macString;
}
// 任務：發送訊息
void ledTask(void *parameter) {
    while (true) {        
        digitalWrite(LED_PIN, BRIGHT);
        delay(100);
        digitalWrite(LED_PIN, DARK);
        delay(100);
        digitalWrite(LED_PIN, BRIGHT);
        delay(100);
        digitalWrite(LED_PIN, DARK);
        delay(700);
    }
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

    currentState = RECEIVED; // 設置狀態為訊息接收
    lastReceivedTime = millis(); // 記錄收到訊息的時間
    if (String((char*)data).indexOf("Fire") >= 0) {
        lastFireTime = millis();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(LED_PIN, DARK);
    
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
        case STANDBY:
            // 接收中狀態，LED 燈閃爍
            if (ledTaskHandle == NULL)
                xTaskCreate(ledTask, "LEDTask", 2048, NULL, 1, &ledTaskHandle);
            break;
        case RECEIVED:
            if (ledTaskHandle != NULL) {
                vTaskDelete(ledTaskHandle); // 刪除任務
                ledTaskHandle = NULL; // 清空任務句柄
            }
            // 收到訊息時，LED 恆亮
            digitalWrite(LED_PIN, BRIGHT);

            // 如果兩秒內沒有接收到其他訊息，則回到接收中狀態
            if (currentTime - lastReceivedTime >= 2500) {
                currentState = STANDBY; // 回到接收中狀態
            }
            break;
    }
    if (currentTime - lastFireTime >= 500) 
    {
        digitalWrite(RELAY_PIN, DARK);
    }
    else if (lastFireTime > 0 && currentTime - lastFireTime < 500)
    {
        digitalWrite(RELAY_PIN, BRIGHT);
    }
    delay(100);
}