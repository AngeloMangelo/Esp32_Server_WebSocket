import asyncio
import websockets
import json
import tkinter as tk
from tkinter import messagebox

ESP32_IP = "192.168.4.1"
#ESP32_IP = "192.168.4.1" 
WS_URL = f"ws://{ESP32_IP}/ws"

class WebSocketClient:
    def __init__(self, gui):
        self.gui = gui
        self.websocket = None
        self.connected = False
        self.loop = asyncio.new_event_loop()

    async def _connect_async(self):
        try:
            self.websocket = await websockets.connect(WS_URL)
            self.connected = True
            self.gui.update_status("Conectado")
            asyncio.create_task(self.listen())
        except Exception as e:
            self.gui.update_status(f"Error: {str(e)}")

    def connect(self):
        # Inicia la corrutina de conexión en el bucle de eventos
        asyncio.run_coroutine_threadsafe(self._connect_async(), self.loop)

    async def listen(self):
        """
        Corrutina que permanece escuchando mensajes entrantes del WebSocket.
        """
        try:
            async for message in self.websocket:
                self.gui.display_message(message, "received")
        except Exception as e:
            self.gui.update_status(f"Desconectado: {str(e)}")
            self.connected = False

    async def _send_async(self, message):
        if self.websocket and self.connected:
            await self.websocket.send(json.dumps(message))
        else:
            self.gui.update_status("No conectado al WebSocket")

    def send(self, message):
        """
        Envía un mensaje JSON al ESP32 y lo muestra en la interfaz (color verde).
        """
        self.gui.display_message(json.dumps(message), "sent")
        asyncio.run_coroutine_threadsafe(self._send_async(message), self.loop)

    async def _disconnect_async(self):
        if self.websocket:
            await self.websocket.close()
            self.connected = False
            self.gui.update_status("Desconectado")

    def disconnect(self):
        asyncio.run_coroutine_threadsafe(self._disconnect_async(), self.loop)


class WebSocketGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 WebSocket GUI")
        self.client = WebSocketClient(self)

        # Etiqueta de estado
        self.status_label = tk.Label(root, text="Desconectado", fg="red")
        self.status_label.pack()

        # Botones de conexión y desconexión
        self.connect_button = tk.Button(root, text="Conectar", command=self.client.connect)
        self.connect_button.pack()

        self.disconnect_button = tk.Button(root, text="Desconectar", command=self.client.disconnect)
        self.disconnect_button.pack()

        # Campo de texto para comandos rápidos
        self.command_entry = tk.Entry(root, width=50)
        self.command_entry.pack()

        self.send_button = tk.Button(root, text="Enviar", command=self.send_command)
        self.send_button.pack()

        # Área de mensajes
        self.response_text = tk.Text(root, height=10, width=50)
        self.response_text.pack()

        # Botones con comandos predefinidos
        self.create_command_buttons()

        # Sección para guardar Config
        self.create_save_config_section()

        # Integramos el bucle Tkinter con asyncio
        self.root.after(100, self.poll_loop)

    def create_command_buttons(self):
        """
        Crea botones para comandos WebSocket comunes.
        """
        commands = ["get_config", "get_sensor", "restart"]
        for cmd in commands:
            button = tk.Button(
                self.root, text=cmd, command=lambda c=cmd: self.send_command(c)
            )
            button.pack()

    def create_save_config_section(self):
        """
        Campo de texto y botón para enviar configuración completa al ESP32.
        """
        tk.Label(self.root, text="JSON de Config:").pack()

        # Cadena completa: web_User y web_Password al inicio
        default_json = """{
  "web_User": "admin",
  "web_Password": "admin",
  "wifi_Ssid": "INFINITUM5BBD_2.4",
  "wifi_Password": "4mDAh4xmFr",
  "ap_Ssid": "MiAccessPoint",
  "ap_Password": "12345678",
  "mqtt_enabled": true,
  "mqtt_Server": "test.mosquitto.org",
  "mqtt_brokerPort": "1883",
  "mqtt_User": "",
  "mqtt_Password": "",
  "mqtt_RootTopic": "tecnm/gve"
}"""
        self.config_entry = tk.Entry(self.root, width=80)
        self.config_entry.insert(0, default_json)
        self.config_entry.pack()

        self.save_button = tk.Button(self.root, text="Guardar Config", command=self.send_save_config)
        self.save_button.pack()

    def send_save_config(self):
        """
        Toma el texto del campo self.config_entry, lo parsea como JSON,
        y lo manda dentro de { "command": "set_config", "value": ... }.
        """
        text = self.config_entry.get()
        if not text.strip():
            messagebox.showwarning("Advertencia", "No se ha proporcionado JSON de configuración.")
            return

        try:
            config_value = json.loads(text)
        except Exception as e:
            messagebox.showerror("Error", f"JSON inválido:\n{e}")
            return

        # Arma el mensaje para set_config
        message = {
            "command": "set_config",
            "value": config_value
        }
        self.client.send(message)

    def send_command(self, command=None):
        """
        Envía un comando simple (sin 'value') al ESP32, p. ej. "get_config".
        """
        if not command:
            command = self.command_entry.get()
        if command:
            self.client.send({"command": command})

    def update_status(self, status):
        """
        Actualiza la etiqueta de estado (Conectado/Desconectado).
        """
        self.status_label.config(
            text=status,
            fg="green" if "Conectado" in status else "red"
        )

    def display_message(self, message, message_type):
        """
        Muestra mensajes en la interfaz, diferenciando enviados vs. recibidos.
        """
        color = "green" if message_type == "sent" else "blue"
        self.response_text.insert(tk.END, message + "\n", message_type)
        self.response_text.tag_config(message_type, foreground=color)
        self.response_text.see(tk.END)

    def poll_loop(self):
        """
        Cada 100 ms, permite que asyncio procese tareas para que `listen()` funcione.
        """
        try:
            self.client.loop.run_until_complete(asyncio.sleep(0.0))
        except Exception:
            pass
        self.root.after(100, self.poll_loop)


if __name__ == "__main__":
    root = tk.Tk()
    app = WebSocketGUI(root)
    root.mainloop()
