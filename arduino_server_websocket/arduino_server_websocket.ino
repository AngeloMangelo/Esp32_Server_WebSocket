#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>
#include <MPU6050.h>
#include <PubSubClient.h>


Preferences prefs;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiClient espClient;
PubSubClient mqttClient(espClient);


String User, Password, wifiSsid, wifiPassword, apSsid, apPassword;
bool isAPMode = false, mqttEnabled;
String mqttServer, mqttPort, mqttUser, mqttPassword, mqttRootTopic;

// Variables del MPU6050
MPU6050 mpu;
int16_t ax, ay, az; // Acelerómetro
int16_t gx, gy, gz; // Giroscopio


const String iotId = String(ESP.getEfuseMac(), HEX);
const String iotName = "ESP32_MPU6050";
const String iotFunction = "Sensor de Movimiento";
const String physicalVariables = "Aceleración (X, Y, Z), Giro (X, Y, Z)";
const String minValues = "Aceleración: -2g, Giro: -250°/s";
const String maxValues = "Aceleración: +2g, Giro: +250°/s";


const unsigned long WIFI_CHECK_INTERVAL = 5000;
const int MAX_RECONNECT_ATTEMPTS = 3;


void loadCredentials() {
    prefs.begin("config", true);
    User = prefs.getString("User", "admin");
    Password = prefs.getString("Pass", "admin");
    wifiSsid = prefs.getString("wifiSsid", "angel");
    wifiPassword = prefs.getString("wifiPass", "1234567890");
    apSsid = prefs.getString("apSsid", "ESP32_IoT");
    apPassword = prefs.getString("apPass", "1234567890");
    mqttEnabled = prefs.getBool("mqttEnable", true);
    mqttServer = prefs.getString("mqttServer", "broker.emqx.io");
    mqttPort = prefs.getString("mqttPort", "1883");
    mqttUser = prefs.getString("mqttUser", "iot");
    mqttPassword = prefs.getString("mqttPass", "1234");
    mqttRootTopic = prefs.getString("mqttRoot", "esp32/mpu6050");
    prefs.end();
}

void saveCredentials() {
    prefs.begin("config", false);
    prefs.putString("User", User);
    prefs.putString("Pass", Password);
    prefs.putString("wifiSsid", wifiSsid);
    prefs.putString("wifiPass", wifiPassword);
    prefs.putString("apSsid", apSsid);
    prefs.putString("apPass", apPassword);
    prefs.putBool("mqttEnable", mqttEnabled);
    prefs.putString("mqttServer", mqttServer);
    prefs.putString("mqttPort", mqttPort);
    prefs.putString("mqttUser", mqttUser);
    prefs.putString("mqttPass", mqttPassword);
    prefs.putString("mqttRoot", mqttRootTopic);
    prefs.end();
    Serial.println("Nuevas credenciales guardadas");
}

void setupAP() {
    if (!isAPMode) {
        Serial.println("Iniciando modo AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSsid.c_str(), apPassword.c_str());
        Serial.println("Punto de acceso creado:");
        Serial.println("SSID: " + apSsid);
        Serial.println("IP: " + WiFi.softAPIP().toString());
        isAPMode = true;
    }
}

bool tryWiFiConnection() {
    Serial.println("Intentando conectar a WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conectado a WiFi");
        Serial.println("IP: " + WiFi.localIP().toString());
        isAPMode = false;
        return true;
    }
    return false;
}

void checkWiFiConnection() {
    if (!isAPMode && WiFi.status() != WL_CONNECTED) {
        Serial.println("Conexión WiFi perdida");

        int reconnectAttempts = 0;
        bool reconnected = false;

        while (reconnectAttempts < MAX_RECONNECT_ATTEMPTS && !reconnected) {
            Serial.printf("Intento de reconexión %d de %d\n", reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
            reconnected = tryWiFiConnection();
            if (!reconnected) {
                reconnectAttempts++;
                delay(1000);
            }
        }

        if (!reconnected) {
            Serial.println("No se pudo reconectar al WiFi después de múltiples intentos");
            setupAP();
        }
    }
}

void setupMQTT() {
    mqttClient.setServer(mqttServer.c_str(), mqttPort.toInt());
    Serial.println("Configurando MQTT...");
}

bool connectMQTT() {
    if (mqttClient.connect(iotId.c_str(), mqttUser.c_str(), mqttPassword.c_str())) {
        Serial.println("Conectado al broker MQTT");
        return true;
    } else {
        Serial.println("Error al conectar al broker MQTT");
        return false;
    }
}

void sendMPU6050Data() {
    DynamicJsonDocument doc(256);
    doc["accelX"] = ax;
    doc["accelY"] = ay;
    doc["accelZ"] = az;
    doc["gyroX"] = gx;
    doc["gyroY"] = gy;
    doc["gyroZ"] = gz;

    String jsonString;
    serializeJson(doc, jsonString);

    ws.textAll(jsonString);

    // Enviar datos por MQTT si hay conexión WiFi y MQTT está habilitado
    if (mqttEnabled && WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
        String topic = mqttRootTopic + "/mpu6050";
        if (mqttClient.publish(topic.c_str(), jsonString.c_str())) {
            Serial.println("Datos publicados por MQTT");
        } else {
            Serial.println("Error al publicar datos por MQTT");
        }
    } else {
        Serial.println("MQTT no está disponible o no hay conexión WiFi");
    }

    Serial.println("Datos del MPU6050:");
    Serial.println("Acelerómetro (X, Y, Z): " + String(ax) + ", " + String(ay) + ", " + String(az));
    Serial.println("Giroscopio (X, Y, Z): " + String(gx) + ", " + String(gy) + ", " + String(gz));
    Serial.println("-----------------------------");
}

String getDeviceInfo() {
    DynamicJsonDocument doc(512);
    doc["id"] = iotId;
    doc["name"] = iotName;
    doc["function"] = iotFunction;
    doc["physicalVariables"] = physicalVariables;
    doc["minValues"] = minValues;
    doc["maxValues"] = maxValues;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String jsonString = String((char*)data);
        Serial.println("Mensaje recibido: " + jsonString);

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, data);

        String command = doc["command"];
        Serial.println("Comando recibido: " + command);

        if (command == "get_config") {
            DynamicJsonDocument response(1024);
            response["User"] = User;
            response["Password"] = Password;
            response["wifi_Ssid"] = wifiSsid;
            response["wifi_Password"] = wifiPassword;
            response["ap_Ssid"] = apSsid;
            response["ap_Password"] = apPassword;
            response["mqtt_enabled"] = mqttEnabled;
            response["mqtt_Server"] = mqttServer;
            response["mqtt_brokerPort"] = mqttPort;
            response["mqtt_User"] = mqttUser;
            response["mqtt_Password"] = mqttPassword;
            response["mqtt_RootTopic"] = mqttRootTopic;

            String responseString;
            serializeJson(response, responseString);
            ws.textAll(responseString);
        } else if (command == "set_config") {
            JsonObject value = doc["value"];
            User = value["User"].as<String>();
            Password = value["Password"].as<String>();
            wifiSsid = value["wifi_Ssid"].as<String>();
            wifiPassword = value["wifi_Password"].as<String>();
            apSsid = value["ap_Ssid"].as<String>();
            apPassword = value["ap_Password"].as<String>();
            mqttEnabled = value["mqtt_enabled"].as<bool>();
            mqttServer = value["mqtt_Server"].as<String>();
            mqttPort = value["mqtt_brokerPort"].as<String>();
            mqttUser = value["mqtt_User"].as<String>();
            mqttPassword = value["mqtt_Password"].as<String>();
            mqttRootTopic = value["mqtt_RootTopic"].as<String>();

            saveCredentials();
            ws.textAll("{\"status\": \"Configuración guardada\"}");
        } else if (command == "restart") {
            ws.textAll("{\"status\": \"Reiniciando...\"}");
            delay(1000);
            ESP.restart();
        } else if (command == "get_info") {
            String info = getDeviceInfo();
            ws.textAll(info);
        }
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void readMPU6050(void *pvParameters) {
    while (1) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando sistema...");

    loadCredentials();

    // Inicializar MPU6050
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 no encontrado");
    } else {
        Serial.println("MPU6050 inicializado");
        xTaskCreatePinnedToCore(readMPU6050, "MPU6050_Task", 10000, NULL, 1, NULL, 1);
    }

    if (!tryWiFiConnection()) {
        setupAP();
    }

    setupMQTT(); 

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "ESP32 WebSocket Server");
    });

    server.begin();
    Serial.println("Servidor WebSocket iniciado");
}

void loop() {
    static unsigned long lastSend = 0;

    if (WiFi.status() != WL_CONNECTED) {
        checkWiFiConnection();
    }

    if (mqttEnabled && WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            Serial.println("Intentando reconectar al broker MQTT...");
            if (connectMQTT()) {
                Serial.println("Reconectado al broker MQTT");
            } else {
                Serial.println("Fallo al reconectar al broker MQTT");
            }
        } else {
            mqttClient.loop(); // Mantener la conexión MQTT activa
        }
    }

    // Publicar datos del MPU6050 cada segundo
    if (millis() - lastSend >= 1000) {
        if (WiFi.status() == WL_CONNECTED) {
            sendMPU6050Data();
        } else {
            Serial.println("No hay conexión WiFi, no se publican datos por MQTT");
        }
        lastSend = millis();
    }
    ws.cleanupClients();
}      