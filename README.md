# ESP32 MPU6050 Sensor con MQTT y WebSocket

Este proyecto utiliza un ESP32 para leer datos de un sensor MPU6050 (acelerómetro y giroscopio) y publicarlos a través de MQTT y WebSocket. Además, el ESP32 puede funcionar en modo AP (punto de acceso) si no puede conectarse a una red WiFi, y sirve una página web embebida para la configuración y el monitoreo.

---

## Características
- 📊 Lectura de datos del sensor MPU6050 (aceleración, giro y ángulo).
- 📡 Publicación de datos en un servidor MQTT.
- 🌐 Servidor WebSocket para comunicación en tiempo real.
- 🔒 Configuración de credenciales WiFi y MQTT almacenadas en la memoria no volátil.
- 📶 Modo AP (punto de acceso) con una página web embebida para la configuración.
- 🖥️ Interfaz web para obtener y actualizar la configuración, y reiniciar el ESP32.

---

## Requisitos
### Hardware
- 🖥️ **ESP32**.
- 🎯 **Sensor MPU6050**.
- 🔌 Conexiones eléctricas adecuadas (alimentación y comunicación I2C).

### Software
- 💻 **Arduino IDE** o **PlatformIO**.
- 📚 Bibliotecas necesarias (ver **Dependencias**).

---

## Dependencias
Este proyecto utiliza las siguientes bibliotecas:
- **WiFi**: Para la conexión WiFi.
- **ESPAsyncWebServer**: Para el servidor web y WebSocket.
- **ArduinoJson**: Para manejar datos en formato JSON.
- **Preferences**: Para almacenar y recuperar configuraciones en la memoria no volátil.
- **MPU6050_tockn**: Para interactuar con el sensor MPU6050.
- **PubSubClient**: Para la comunicación MQTT.

Puedes instalar estas bibliotecas a través del administrador de bibliotecas de Arduino IDE o PlatformIO.

---

## Configuración
1. **Conexiones del MPU6050**:
   - Conecta el sensor MPU6050 al ESP32 mediante I2C:
     - `VCC` a `3.3V`.
     - `GND` a `GND`.
     - `SCL` a `GPIO 22` (SCL del ESP32).
     - `SDA` a `GPIO 21` (SDA del ESP32).

2. **Credenciales WiFi y MQTT**:
   - Modifica las credenciales predeterminadas en el código o utiliza el modo AP para configurarlas a través de la página web.

3. **Broker MQTT**:
   - Asegúrate de tener un broker MQTT en funcionamiento (por ejemplo, `broker.emqx.io`).
   - Configura el servidor MQTT, el puerto y las credenciales en el código o a través de la página web.

---

## Uso del Modo AP y Página Web
Cuando el ESP32 no puede conectarse a una red WiFi, entra en modo AP (punto de acceso). En este modo, puedes conectarte al ESP32 desde un navegador para configurarlo.

### Pasos para Acceder a la Página Web
1. Conéctate al punto de acceso del ESP32:
   - **SSID**: `ESP32_IoT` (o el que hayas configurado).
   - **Contraseña**: `1234567890` (o la que hayas configurado).

2. Abre un navegador y visita la dirección IP del ESP32:
   - **Dirección IP**: `192.168.4.1`.

3. Usa la página web para:
   - **Obtener la configuración actual**.
   - **Actualizar la configuración** (WiFi, MQTT, etc.).
   - **Reiniciar el ESP32**.

---

## Interfaz Web
La página web embebida permite:
- **Obtener Configuración Actual**: Muestra la configuración actual del ESP32.
- **Actualizar Configuración**: Permite modificar las credenciales WiFi y MQTT.
- **Reiniciar ESP32**: Reinicia el dispositivo.
- **Consultar datos del sensor**: Para poder ver si se estan mandando correctamente los datos del sensor  sus especificaciones.

![pagina](https://github.com/user-attachments/assets/9850948e-73ab-48a2-a034-76872b008684)


### Ejemplo de Comandos
- **Obtener Configuración Actual**:
  ```json
  {
    "command": "get_config"
  }

- **Actualizar Configuración**:
  ```json
  {
  "command": "set_config",
  "value": {
    "wifi_Ssid": "MiWiFi",
    "wifi_Password": "12345678",
    "mqtt_Server": "broker.emqx.io",
    "mqtt_brokerPort": "1883",
    "mqtt_User": "usuario",
    "mqtt_Password": "contraseña",
    "mqtt_RootTopic": "esp32"
  }

- **Reiniciar ESP32**:
  ```json
  {
    "command": "restart"
  }

## Instrucciones de Uso
1. Carga el código en tu ESP32.
2. Conéctate a la red WiFi configurada o al modo AP del ESP32.
3. Usa la página web para configurar y monitorear el ESP32.
4. Verifica que los datos del MPU6050 se publiquen en el tópico MQTT especificado (`mqttRootTopic + "/mpu6050"`).

---

## Estructura del Código
- **`loadCredentials()`**: Carga las credenciales desde la memoria no volátil.
- **`saveCredentials()`**: Guarda las credenciales en la memoria no volátil.
- **`setupAP()`**: Configura el ESP32 como punto de acceso y sirve la página web.
- **`tryWiFiConnection()`**: Intenta conectar a una red WiFi.
- **`checkWiFiConnection()`**: Verifica y maneja la reconexión WiFi.
- **`sendMPU6050Data()`**: Envía los datos del MPU6050 a través de WebSocket y MQTT.
- **`setupMQTT()`**: Configura el cliente MQTT.
- **`connectMQTT()`**: Conecta al broker MQTT.
- **`handleWebSocketMessage()`**: Maneja los mensajes recibidos a través de WebSocket.

---

## Ejemplo de Datos Publicados
Los datos del MPU6050 se publican en formato JSON:
```json
{
  "accelX": 123.45,
  "accelY": 456.78,
  "accelZ": 789.01,
  "gyroX": 12.34,
  "gyroY": 56.78,
  "gyroZ": 90.12,
  "angleX": 45.67,
  "angleY": 89.01,
  "angleZ": 23.45
}
