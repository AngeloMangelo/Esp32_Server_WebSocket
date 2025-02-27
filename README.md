# 🚀 ESP32 MPU6050 Sensor con MQTT y WebSocket

Este proyecto utiliza un **ESP32** para leer datos de un sensor **MPU6050** (acelerómetro y giroscopio) y publicarlos a través de **MQTT** y **WebSocket**. Además, el ESP32 puede funcionar en modo **AP** (punto de acceso) si no puede conectarse a una red WiFi.

---

## 🌟 Características
- 📊 Lectura de datos del sensor MPU6050 (aceleración y giro).
- 📡 Publicación de datos en un servidor MQTT.
- 🌐 Servidor WebSocket para comunicación en tiempo real.
- 🔒 Configuración de credenciales WiFi y MQTT almacenadas en la memoria no volátil del ESP32.
- 📶 Modo AP (punto de acceso) para configuración en caso de falta de conexión WiFi.

---

## 🛠️ Requisitos
### Hardware
- 🖥️ **ESP32**.
- 🎯 **Sensor MPU6050**.
- 🔌 Conexiones eléctricas adecuadas (alimentación y comunicación I2C).

### Software
- 💻 **Arduino IDE** o **PlatformIO**.
- 📚 Bibliotecas necesarias (ver **Dependencias**).

---

## 📦 Dependencias
Este proyecto utiliza las siguientes bibliotecas:
- **WiFi**: Para la conexión WiFi.
- **ESPAsyncWebServer**: Para el servidor web y WebSocket.
- **ArduinoJson**: Para manejar datos en formato JSON.
- **Preferences**: Para almacenar y recuperar configuraciones en la memoria no volátil.
- **MPU6050**: Para interactuar con el sensor MPU6050.
- **PubSubClient**: Para la comunicación MQTT.

Puedes instalar estas bibliotecas a través del administrador de bibliotecas de Arduino IDE o PlatformIO.

---

## 🔧 Configuración
1. **Conexiones del MPU6050**:
   - Conecta el sensor MPU6050 al ESP32 mediante I2C:
     - `VCC` a `3.3V`.
     - `GND` a `GND`.
     - `SCL` a `GPIO 22` (SCL del ESP32).
     - `SDA` a `GPIO 21` (SDA del ESP32).

2. **Credenciales WiFi y MQTT**:
   - Modifica las credenciales predeterminadas en el código o utiliza el modo AP para configurarlas a través de WebSocket.

3. **Broker MQTT**:
   - Asegúrate de tener un broker MQTT en funcionamiento (por ejemplo, `broker.emqx.io`).
   - Configura el servidor MQTT, el puerto y las credenciales en el código.

---

## 🚀 Instrucciones de Uso
1. Carga el código en tu ESP32.
2. Conecta el ESP32 a una red WiFi o accede al modo AP para configurar las credenciales.
3. Verifica que los datos del MPU6050 se publiquen en el tópico MQTT especificado (`mqttRootTopic + "/mpu6050"`).
4. Usa un cliente MQTT (como MQTT Explorer o Mosquitto) para suscribirte al tópico y ver los datos.

---

## 🧩 Estructura del Código
- **`loadCredentials()`**: Carga las credenciales desde la memoria no volátil.
- **`saveCredentials()`**: Guarda las credenciales en la memoria no volátil.
- **`setupAP()`**: Configura el ESP32 como punto de acceso.
- **`tryWiFiConnection()`**: Intenta conectar a una red WiFi.
- **`checkWiFiConnection()`**: Verifica y maneja la reconexión WiFi.
- **`sendMPU6050Data()`**: Envía los datos del MPU6050 a través de WebSocket y MQTT.
- **`setupMQTT()`**: Configura el cliente MQTT.
- **`connectMQTT()`**: Conecta al broker MQTT.
- **`handleWebSocketMessage()`**: Maneja los mensajes recibidos a través de WebSocket.

---

## 📊 Ejemplo de Datos Publicados
Los datos del MPU6050 se publican en formato JSON:
```json
{
  "accelX": 123,
  "accelY": 456,
  "accelZ": 789,
  "gyroX": 101,
  "gyroY": 102,
  "gyroZ": 103
}
