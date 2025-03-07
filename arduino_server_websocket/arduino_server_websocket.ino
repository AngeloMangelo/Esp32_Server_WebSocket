#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>
#include <Wire.h>
#include <MPU6050_tockn.h> 
#include <PubSubClient.h> 


Preferences prefs;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Variables de configuración
String User, Password, wifiSsid, wifiPassword, apSsid, apPassword;
bool isAPMode = false, mqttEnabled;
String mqttServer, mqttPort, mqttUser, mqttPassword, mqttRootTopic;

// Variables del MPU6050
MPU6050 mpu6050(Wire); // Objeto MPU6050 con la nueva librería
bool mpuConnected = false; // Bandera para verificar si el MPU6050 está conectado

// Información del dispositivo
const String iotId = String(ESP.getEfuseMac(), HEX);
const String iotName = "ESP32_MPU6050";
const String iotFunction = "Sensor de Movimiento";
const String physicalVariables = "Aceleración (X, Y, Z), Giro (X, Y, Z), Ángulo (X, Y, Z)";
const String minValues = "Aceleración: -2g, Giro: -250°/s, Ángulo: -180°";
const String maxValues = "Aceleración: +2g, Giro: +250°/s, Ángulo: +180°";

// Constantes
const unsigned long WIFI_CHECK_INTERVAL = 5000;
const int MAX_RECONNECT_ATTEMPTS = 3;

// Funciones
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
    mqttRootTopic = prefs.getString("mqttRoot", "esp32");
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

bool initializeMPU6050() {
    Wire.begin(); // Reiniciar el bus I2C
    mpu6050.begin();
    delay(100); // Pequeño retardo para estabilizar la comunicación

    // Verificar si el MPU6050 responde
    Wire.beginTransmission(0x68); // Dirección I2C del MPU6050
    if (Wire.endTransmission() == 0) {
        mpu6050.calcGyroOffsets(true); // Calibración del giroscopio
        Serial.println("MPU6050 inicializado correctamente");
        mpuConnected = true;
        return true;
    } else {
        Serial.println("Error al inicializar el MPU6050. Verifica la conexión.");
        mpuConnected = false;
        return false;
    }
}

void sendMPU6050Data() {
    if (!mpuConnected) {
        Serial.println("MPU6050 no está conectado. Intentando reinicializar...");
        if (!initializeMPU6050()) {
            Serial.println("No se pudo reinicializar el MPU6050");
            return;
        }
    }

    DynamicJsonDocument doc(256);
    doc["accelX"] = mpu6050.getAccX();
    doc["accelY"] = mpu6050.getAccY();
    doc["accelZ"] = mpu6050.getAccZ();
    doc["gyroX"] = mpu6050.getGyroX();
    doc["gyroY"] = mpu6050.getGyroY();
    doc["gyroZ"] = mpu6050.getGyroZ();
    doc["angleX"] = mpu6050.getAngleX();
    doc["angleY"] = mpu6050.getAngleY();
    doc["angleZ"] = mpu6050.getAngleZ();

    String jsonString;
    serializeJson(doc, jsonString);

    // Enviar datos por WebSocket
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
    Serial.println("Acelerómetro (X, Y, Z): " + String(mpu6050.getAccX()) + ", " + String(mpu6050.getAccY()) + ", " + String(mpu6050.getAccZ()));
    Serial.println("Giroscopio (X, Y, Z): " + String(mpu6050.getGyroX()) + ", " + String(mpu6050.getGyroY()) + ", " + String(mpu6050.getGyroZ()));
    Serial.println("Ángulo (X, Y, Z): " + String(mpu6050.getAngleX()) + ", " + String(mpu6050.getAngleY()) + ", " + String(mpu6050.getAngleZ()));
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

String getSensorData() {
    DynamicJsonDocument doc(256);
    doc["accelX"] = mpu6050.getAccX();
    doc["accelY"] = mpu6050.getAccY();
    doc["accelZ"] = mpu6050.getAccZ();
    doc["gyroX"] = mpu6050.getGyroX();
    doc["gyroY"] = mpu6050.getGyroY();
    doc["gyroZ"] = mpu6050.getGyroZ();
    doc["angleX"] = mpu6050.getAngleX();
    doc["angleY"] = mpu6050.getAngleY();
    doc["angleZ"] = mpu6050.getAngleZ();

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
        } else if (command == "get_sensor") {
            String sensorData = getSensorData();
            ws.textAll(sensorData);
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
        if (mpuConnected) {
            mpu6050.update(); // Actualizar datos del MPU6050
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Pequeño retardo para evitar sobrecargar el núcleo
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando sistema...");

    loadCredentials();

    // Inicializar MPU6050
    if (!initializeMPU6050()) {
        Serial.println("Error al inicializar el MPU6050. Verifica la conexión.");
    }

    // Crear una tarea para leer el MPU6050 en el segundo núcleo (Core 0)
    xTaskCreatePinnedToCore(
        readMPU6050,    // Función que implementa la tarea
        "MPU6050_Task", // Nombre de la tarea
        10000,          // Tamaño de la pila (en palabras)
        NULL,           // Parámetros de la tarea
        1,              // Prioridad de la tarea
        NULL,           // Manejador de la tarea
        0               // Núcleo donde se ejecutará (0 para Core 0)
    );

    if (!tryWiFiConnection()) {
        setupAP();
    }

    setupMQTT(); // Configurar MQTT

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

    // Verificar conexión WiFi
    if (WiFi.status() != WL_CONNECTED) {
        checkWiFiConnection();
    }

    // Verificar conexión MQTT si está habilitado
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