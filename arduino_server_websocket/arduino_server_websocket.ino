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





const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuración ESP32</title>
    <style>
        :root {
            --primary:#424669 ;
            --secondary: #2E3150;
            --text: #afafaf;
            --card: #2E3150;
            --danger: #F44336;
        }

        body {
            font-family: Arial, sans-serif;
            margin: 0;
            background-color: var(--secondary);
            color: var(--text);
            min-height: 100vh;
            overflow: hidden; /* Evitar scroll antes del login */
        }

        /* Menú lateral */
        .sidebar {
            width: 250px;
            background: var(--card);
            padding: 1.5rem;
            position: fixed;
            height: 100vh;
            overflow-y: auto;
            transition: transform 0.3s ease;
            z-index: 1000;
            display: none; /* Ocultar inicialmente */
        }

        .sidebar.visible {
            display: block; /* Mostrar solo después de login */
        }

        .sidebar.hidden {
            transform: translateX(-100%);
        }

        .sidebar-toggle {
            position: fixed;
            left: 10px;
            top: 10px;
            z-index: 1001;
            background: var(--primary);
            color: white;
            border: none;
            padding: 10px;
            border-radius: 5px;
            cursor: pointer;
            display: none; /* Ocultar inicialmente */
        }

        .sidebar h2 {
            color: white;
            text-align: center;
            margin-bottom: 2rem;
        }

        .sidebar ul {
            list-style: none;
            padding: 0;
        }

        .sidebar ul li {
            margin-bottom: 1rem;
        }

        .sidebar ul li a {
            color: white;
            text-decoration: none;
            display: block;
            padding: 0.8rem;
            border-radius: 8px;
            transition: background-color 0.3s ease;
        }

        .sidebar ul li a:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }

        .sidebar ul li a.active {
            background-color: rgba(255, 255, 255, 0.2);
        }

        /* Contenido principal */
        .main-content {
            background-color: #2E3150;
            margin-left: 250px;
            padding: 2rem;
            transition: margin-left 0.3s ease;
            display: none; /* Ocultar inicialmente */
        }

        .section {
            background-color: #424669;
            padding: 2rem;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
            margin-bottom: 2rem;
        }

        .hidden {
            display: none;
        }

        /* Formularios */
        .form-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 1.5rem;
        }

        .input-group {
            margin-bottom: 1.5rem;
        }

        label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 600;
        }

        input {
            width: 100%;
            padding: 0.8rem;
            border: 2px solid #ddd;
            border-radius: 8px;
            transition: border-color 0.3s ease;
        }

        input:focus {
            outline: none;
            border-color: #008cdd;
        }

        button {
            padding: 0.8rem 1.5rem;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
        }

        .btn-primary {
            background-color: var(--primary);
            color: white;
        }

        .btn-secondary {
            background-color: var(--secondary);
            color: white;
        }

        .btn-danger {
            background-color: var(--danger);
            color: white;
        }

        button:hover {
            opacity: 0.9;
            transform: translateY(-2px);
        }

        /* Respuestas */
        #response-section pre {
            background: var(--card);
            padding: 1.5rem;
            border-radius: 8px;
            white-space: pre-wrap;
            font-family: monospace;
        }

        /* Login */
        .login-container {
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: 2000;
        }

        .login-box {
            background: var(--card);
            padding: 2rem;
            border-radius: 15px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            width: 90%;
            max-width: 400px;
        }

        .reset-section {
            margin-top: 2rem;
            padding-top: 2rem;
            border-top: 1px solid rgba(255, 255, 255, 0.1);
        }

        /* Responsive */
        @media (max-width: 768px) {
            .main-content {
                margin-left: 0;
                padding: 1rem;
            }

            .sidebar {
                transform: translateX(-100%);
            }

            .sidebar.visible {
                transform: translateX(0);
            }
        }
    </style>
</head>
<body>
    <!-- Login -->
    <div id="loginContainer" class="login-container">
        <div class="login-box">
            <h2>Acceso Seguro</h2>
            <form id="loginForm">
                <div class="input-group">
                    <label for="password">Código de Seguridad:</label>
                    <input type="password" id="password" required autocomplete="off">
                </div>
                <button type="submit" class="btn-primary">Ingresar</button>
            </form>
            <p id="error-message" style="color: #F44336; display: none;">Código incorrecto</p>
        </div>
    </div>

    <!-- Menú -->
    <button class="sidebar-toggle" onclick="toggleSidebar()">☰</button>
    <div class="sidebar">
        <h2>Menú</h2>
        <ul>
            <li><a href="#" data-section="config" class="active">Configuración</a></li>
            <li><a href="#" data-section="info">Información</a></li>
            <li><a href="#" data-section="sensor">Sensor</a></li>
            <li><a href="#" data-section="responses">Respuestas</a></li>
            <li><a href="#" data-section="restart">Reiniciar</a></li>
        </ul>
        <button class="btn-secondary" onclick="toggleSidebar()" style="margin-top: 2rem; width: 100%;">
            Ocultar Menú
        </button>
    </div>

    <!-- Contenido principal -->
    <div class="main-content">
        <!-- Configuración -->
        <div id="config" class="section">
            <h1>Configuración del Dispositivo</h1>
            <div class="form-grid">
                <div>
                    <form id="getConfigForm">
                        <button class="btn-secondary" type="submit">Obtener Configuración</button>
                    </form>
                </div>
                <div>
                    <form id="setConfigForm">
                        <div class="input-group">
                            <label>WiFi SSID:</label>
                            <input type="text" id="wifiSsid" required>
                        </div>
                        <div class="input-group">
                            <label>Contraseña WiFi:</label>
                            <input type="password" id="wifiPassword" required>
                        </div>
                        <div class="input-group">
                            <label>Servidor MQTT:</label>
                            <input type="text" id="mqttServer" required>
                        </div>
                        <div class="input-group" style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                            <div>
                                <label>Puerto MQTT:</label>
                                <input type="text" id="mqttPort" required>
                            </div>
                            <div>
                                <label>Tema Principal:</label>
                                <input type="text" id="mqttRootTopic" required>
                            </div>
                        </div>
                        <div class="input-group">
                            <label>Usuario MQTT:</label>
                            <input type="text" id="mqttUser" required>
                        </div>
                        <div class="input-group">
                            <label>Contraseña MQTT:</label>
                            <input type="password" id="mqttPassword" required>
                        </div>
                        <button class="btn-primary" type="submit">Guardar Configuración</button>
                    </form>

                    <!-- Sección de Reset a Valores por Defecto -->
                    <div class="reset-section">
                        <h3>Restablecer Configuración</h3>
                        <p>Esta acción reseteará todos los parámetros a los valores por defecto.</p>
                        <button id="resetDefaults" class="btn-danger">Restablecer a Valores por Defecto</button>
                    </div>
                </div>
            </div>
        </div>

        <!-- Otras secciones -->
        <div id="info" class="section hidden">
            <h1>Información del Dispositivo</h1>
            <form id="getInfoForm">
                <button class="btn-secondary" type="submit">Actualizar Información</button>
            </form>
        </div>

        <div id="sensor" class="section hidden">
            <h1>Datos del Sensor</h1>
            <form id="getSensorForm">
                <button class="btn-secondary" type="submit">Obtener Datos</button>
            </form>
        </div>

        <div id="responses" class="section hidden">
            <h1>Respuestas del ESP32</h1>
            <pre id="output">Esperando datos...</pre>
        </div>

        <div id="restart" class="section hidden">
            <h1>Reinicio del Dispositivo</h1>
            <form id="restartForm">
                <button class="btn-secondary" type="submit">Confirmar Reinicio</button>
            </form>
        </div>
    </div>

    <script>
        const SECURITY_CODE = "88888888";
        let menuVisible = true;

        // Manejar login
        document.getElementById('loginForm').addEventListener('submit', (e) => {
            e.preventDefault();
            const password = document.getElementById('password').value;
            
            if (password === SECURITY_CODE) {
                document.getElementById('loginContainer').style.display = 'none';
                document.querySelector('.sidebar').classList.add('visible');
                document.querySelector('.main-content').style.display = 'block';
                document.querySelector('.sidebar-toggle').style.display = 'block';
                
                // Mostrar sección de configuración por defecto
                document.getElementById('config').classList.remove('hidden');
            } else {
                document.getElementById('error-message').style.display = 'block';
                // Limpiar campo de contraseña
                document.getElementById('password').value = '';
                // Enfocar campo nuevamente
                document.getElementById('password').focus();
            }
        });

        // Toggle menú
        function toggleSidebar() {
            menuVisible = !menuVisible;
            document.querySelector('.sidebar').classList.toggle('hidden');
            document.querySelector('.main-content').style.marginLeft = menuVisible ? '250px' : '0';
        }

        // Navegación
        document.querySelectorAll('.sidebar a').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const sectionId = link.getAttribute('data-section');
                
                // Actualizar menú
                document.querySelectorAll('.sidebar a').forEach(a => a.classList.remove('active'));
                link.classList.add('active');
                
                // Mostrar sección
                document.querySelectorAll('.section').forEach(section => {
                    section.classList.add('hidden');
                });
                document.getElementById(sectionId).classList.remove('hidden');
            });
        });

        // WebSocket y lógica de comunicación con el ESP32
        const socket = new WebSocket('ws://' + window.location.hostname + '/ws');

        socket.addEventListener('open', (event) => {
            console.log('Conexión WebSocket establecida');
        });

        socket.addEventListener('message', (event) => {
            const output = document.getElementById('output');
            const data = JSON.parse(event.data);

            // Si la respuesta es la configuración actual, rellenar el formulario
            if (data.User !== undefined) {
                document.getElementById('wifiSsid').value = data.wifi_Ssid || "";
                document.getElementById('wifiPassword').value = data.wifi_Password || "";
                document.getElementById('mqttServer').value = data.mqtt_Server || "";
                document.getElementById('mqttPort').value = data.mqtt_brokerPort || "";
                document.getElementById('mqttUser').value = data.mqtt_User || "";
                document.getElementById('mqttPassword').value = data.mqtt_Password || "";
                document.getElementById('mqttRootTopic').value = data.mqtt_RootTopic || "";
            }

            // Mostrar la respuesta en el área de salida
            output.textContent = JSON.stringify(data, null, 2);
        });

        socket.addEventListener('error', (event) => {
            console.error('Error en la conexión WebSocket:', event);
        });

        socket.addEventListener('close', (event) => {
            console.log('Conexión WebSocket cerrada');
        });

        // Obtener la configuración actual
        document.getElementById('getConfigForm').addEventListener('submit', (event) => {
            event.preventDefault();
            const message = JSON.stringify({ command: "get_config" });
            socket.send(message);
        });

        // Actualizar la configuración
        document.getElementById('setConfigForm').addEventListener('submit', (event) => {
            event.preventDefault();
            const config = {
                command: "set_config",
                value: {
                    User: "admin",
                    Password: "admin",
                    wifi_Ssid: document.getElementById('wifiSsid').value,
                    wifi_Password: document.getElementById('wifiPassword').value,
                    ap_Ssid: "ESP32_IoT",
                    ap_Password: "1234567890",
                    mqtt_enabled: true,
                    mqtt_Server: document.getElementById('mqttServer').value,
                    mqtt_brokerPort: document.getElementById('mqttPort').value,
                    mqtt_User: document.getElementById('mqttUser').value,
                    mqtt_Password: document.getElementById('mqttPassword').value,
                    mqtt_RootTopic: document.getElementById('mqttRootTopic').value
                }
            };
            const message = JSON.stringify(config);
            socket.send(message);
        });

        // Obtener información del dispositivo
        document.getElementById('getInfoForm').addEventListener('submit', (event) => {
            event.preventDefault();
            const message = JSON.stringify({ command: "get_info" });
            socket.send(message);
        });

        // Obtener datos del sensor
        document.getElementById('getSensorForm').addEventListener('submit', (event) => {
            event.preventDefault();
            const message = JSON.stringify({ command: "get_sensor" });
            socket.send(message);
        });

        // Reiniciar el ESP32
        document.getElementById('restartForm').addEventListener('submit', (event) => {
            event.preventDefault();
            const message = JSON.stringify({ command: "restart" });
            socket.send(message);
        });

        // Resetear a valores por defecto
        document.getElementById('resetDefaults').addEventListener('click', (event) => {
            event.preventDefault();
            if (confirm('¿Estás seguro de que deseas restablecer todos los parámetros a los valores por defecto?')) {
                const message = JSON.stringify({ command: "reset_defaults" });
                socket.send(message);
            }
        });
        
        // Enfocar campo de contraseña al cargar
        window.onload = function() {
            document.getElementById('password').focus();
        };
    </script>
</body>
</html>
)rawliteral";




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

    // Servir la página web
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html);
        });
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