#include <esp_now.h>
#include <WiFi.h>

#define CHANNEL 1
#define BUTTON_PIN 0 // 按鈕連接的 GPIO 引腳

enum State {
    IDLE, // 待機狀態
    FIRE // 開火狀態
};

// 接收端的 MAC 地址陣列
uint8_t peers[][6] = {
    {0x10, 0x20, 0xBA, 0xAD, 0xBE, 0x40},
    {0x10, 0x20, 0xBA, 0xAD, 0xC0, 0xE8},
    {0x10, 0x20, 0xBA, 0xAD, 0xC0, 0x68},
    {0x10, 0x20, 0xBA, 0xAD, 0xC2, 0x10}
};

State currentState = IDLE; // 初始狀態
TaskHandle_t sendTaskHandle = NULL; // 任務句柄

// 函數：格式化 MAC 地址為字符串
String formatMacAddress(const uint8_t *mac_addr) {
    String macString = "";
    for (int i = 0; i < 6; i++) {
        macString += String(mac_addr[i], HEX);
        if (i < 5) macString += ":"; // 在每個字節之間添加冒號
    }
    return macString;
}

// 更新回調函數的簽名
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
    Serial.print("Last Packet Send Status: ");
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.print("Delivery Success to: ");
    } else {
        Serial.print("Delivery Fail to: ");
    }
    Serial.println(formatMacAddress(info->des_addr)); // 使用格式化函數
}

// 子程式：發送訊息
void sendMessage(const char *message) {
    for (int i = 0; i < sizeof(peers) / sizeof(peers[0]); i++) {
        esp_now_send(peers[i], (uint8_t *)message, strlen(message));
    }
    Serial.println("Message sent: \"" + String(message) + "\" on CHANNEL " + CHANNEL);
}

// 任務：發送訊息
void sendTask(void *parameter) {
    while (true) {
        if (currentState == IDLE) {
            sendMessage("Hello ESP-NOW");
            vTaskDelay(2000 / portTICK_PERIOD_MS); // 每 2 秒發送一次
        }
    }
}
// 子程式：檢查按鈕狀態並切換狀態
State checkButtonState() {
    int buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState == LOW) { // 按下時，GPIO 為 LOW
        currentState = FIRE;
    } else {
        currentState = IDLE;
    }
    return currentState;
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Serial.println("ESP-NOW Sender");

    // 印出自身的 MAC 地址
    Serial.print("Master MAC Address: ");
    Serial.println(WiFi.macAddress());

    pinMode(BUTTON_PIN, INPUT_PULLUP); // 設定按鈕引腳為上拉輸入

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent); // 註冊發送回調函數

    esp_now_peer_info_t peerInfo;

    // 循環添加所有接收端到 peer 列表
    for (int i = 0; i < sizeof(peers) / sizeof(peers[0]); i++) {
        memcpy(peerInfo.peer_addr, peers[i], 6);
        peerInfo.channel = CHANNEL;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add peer");
        }
    }
}

void loop() {
    switch (checkButtonState()) { // 檢查按鈕狀態並更新當前狀態
        case IDLE:
            if (sendTaskHandle == NULL)
                xTaskCreate(sendTask, "SendTask", 2048, NULL, 1, &sendTaskHandle);
            break;
        case FIRE:
            if (sendTaskHandle != NULL) {
                vTaskDelete(sendTaskHandle); // 刪除任務
                sendTaskHandle = NULL; // 清空任務句柄
            }
            // 調用發送訊息的子程式
            sendMessage("Fire ESP-NOW");
            delay(50);
            break;
    }
}