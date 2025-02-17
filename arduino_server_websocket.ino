#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_system.h>

Preferences prefs;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String User;
String Password;
String wifiSsid;
String wifiPassword;
String apSsid;
String apPassword;
bool isAPMode = false;
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; 
const int MAX_RECONNECT_ATTEMPTS = 3;
bool mqttEnabled;
String mqttServer, mqttPort, mqttUser, mqttPassword, mqttRootTopic; 

void loadCredentials() {
    prefs.begin("config", true);
    User = prefs.getString("User", "admin");
    Password = prefs.getString("Pass", "admin");
    wifiSsid = prefs.getString("wifiSsid", "");
    wifiPassword = prefs.getString("wifiPass", "");
    apSsid = prefs.getString("apSsid", "ESP32_IoT");
    apPassword = prefs.getString("apPass", "1234567890");
    mqttEnabled = prefs.getBool("mqttEnable", false);
    mqttServer = prefs.getString("mqttServer", "");
    mqttPort = prefs.getString("mqttPort", "1883");
    mqttUser = prefs.getString("mqttUser", "");
    mqttPassword = prefs.getString("mqttPass", "");
    mqttRootTopic = prefs.getString("mqttRoot", "");

    String esp32Id = String(ESP.getEfuseMac(), HEX); 
    String macAddress = WiFi.macAddress();
    uint32_t freeHeap = ESP.getFreeHeap(); 

    Serial.println("\nInformación del ESP32:"); 
    Serial.println("---------------------");
    Serial.println("ID del ESP32: " + esp32Id);
    Serial.println("Dirección MAC: " + macAddress);
    Serial.println("Memoria libre (Heap): " + String(freeHeap) + " bytes");
    Serial.println("---------------------\n");

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
        }
        else if (command == "set_config") { 
            JsonObject value = doc["value"];

            String oldUser = User;
            String oldPassword = Password;         
            String oldWifiSsid = wifiSsid;          
            String oldWifiPassword = wifiPassword;
            String oldApSsid = apSsid;
            String oldApPassword = apPassword;
            bool oldMqttEnabled = mqttEnabled;
            String oldMqttServer = mqttServer;
            String oldMqttPort = mqttPort;
            String oldMqttUser = mqttUser;
            String oldMqttPassword = mqttPassword;
            String oldMqttRootTopic = mqttRootTopic;

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

            // UPD DT
            Serial.println("Cambios detectados en la configuración:");           
            if (User != oldUser) Serial.println(" - Usuario: " + oldUser + " -> " + User);            
            if (Password != oldPassword) Serial.println(" - Contraseña: " + oldPassword + " -> " + Password);
            if (wifiSsid != oldWifiSsid) Serial.println(" - SSID WiFi: " + oldWifiSsid + " -> " + wifiSsid);
            if (wifiPassword != oldWifiPassword) Serial.println(" - Contraseña WiFi: " + oldWifiPassword + " -> " + wifiPassword);
            if (apSsid != oldApSsid) Serial.println(" - SSID AP: " + oldApSsid + " -> " + apSsid);
            if (apPassword != oldApPassword) Serial.println(" - Contraseña AP: " + oldApPassword + " -> " + apPassword);
            if (mqttEnabled != oldMqttEnabled) Serial.println(" - MQTT Habilitado: " + String(oldMqttEnabled ? "Sí" : "No") + " -> " + String(mqttEnabled ? "Sí" : "No"));
            if (mqttServer != oldMqttServer) Serial.println(" - Servidor MQTT: " + oldMqttServer + " -> " + mqttServer);
            if (mqttPort != oldMqttPort) Serial.println(" - Puerto MQTT: " + oldMqttPort + " -> " + mqttPort);
            if (mqttUser != oldMqttUser) Serial.println(" - Usuario MQTT: " + oldMqttUser + " -> " + mqttUser);
            if (mqttPassword != oldMqttPassword) Serial.println(" - Contraseña MQTT: " + oldMqttPassword + " -> " + mqttPassword);
            if (mqttRootTopic != oldMqttRootTopic) Serial.println(" - Tema raíz MQTT: " + oldMqttRootTopic + " -> " + mqttRootTopic);

            saveCredentials();
            ws.textAll("{\"status\": \"Configuración guardada\"}");
        }
        else if (command == "restart") {
            ws.textAll("{\"status\": \"Reiniciando...\"}");                        
            delay(1000);
            ESP.restart();
        }
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {    // Papas con queso
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            Serial.printf("Mensaje recibido del cliente #%u: %.*s\n", client->id(), len, data);  
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
            Serial.println("Pong recibido");
            break;
        case WS_EVT_ERROR:
            Serial.println("Error en WebSocket");
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando sistema...");
    
    loadCredentials();

    Serial.println("\nConfiguración obtenida ESP32:");
    Serial.println("-----------------------");
    Serial.println("Usuario: " + User);
    Serial.println("Contraseña: " + Password);
    Serial.println("SSID WiFi: " + wifiSsid);
    Serial.println("Contraseña WiFi: " + wifiPassword);
    Serial.println("SSID AP: " + apSsid);
    Serial.println("Contraseña AP: " + apPassword);
    Serial.println("MQTT Habilitado: " + String(mqttEnabled ? "Sí" : "No")); 
    Serial.println("Servidor MQTT: " + mqttServer);
    Serial.println("Puerto MQTT: " + mqttPort);
    Serial.println("Usuario MQTT: " + mqttUser);
    Serial.println("Contraseña MQTT: " + mqttPassword);      
    Serial.println("Tema raíz MQTT: " + mqttRootTopic);
    Serial.println("-----------------------\n");
    
    if (!tryWiFiConnection()) {   
        setupAP();               
    }
    
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){  
        request->send(200, "text/plain", "ESP32 WebSocket Server");  
    });
    
    server.begin();
    Serial.println("Servidor WebSocket iniciado");
}

void loop() {
    unsigned long currentMillis = millis();               
    if (currentMillis - lastWifiCheck >= WIFI_CHECK_INTERVAL) {   
        checkWiFiConnection();
        lastWifiCheck = currentMillis;
    }
    ws.cleanupClients();
}