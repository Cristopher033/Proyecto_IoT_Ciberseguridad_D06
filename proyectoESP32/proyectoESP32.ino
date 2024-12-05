#include <WiFi.h>
#include <WebServer.h>

// Configuración de red WiFi
const char* ssid = "nombre-wifi";
const char* password = "clave-wifi";

WebServer server(80);

// Pines para comunicación Serial2
#define RXD2 16
#define TXD2 17

// Contadores de colores
int contadorRojo = 0;
int contadorVerde = 0;
int contadorAzul = 0;
int contadorDesconocido = 0;

// Estado del clasificador
bool clasificadorActivo = false;

void setup() {
    Serial.begin(9600);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado a WiFi");

    // Configurar rutas del servidor web
    server.on("/", handleRoot);
    server.on("/reset", handleReset);
    server.on("/update", handleUpdate);
    server.on("/toggle", handleToggle);
    server.begin();

    Serial.println("Servidor HTTP iniciado");
}

void loop() {
    server.handleClient();

    // Leer datos del Arduino si el clasificador está activo
    if (clasificadorActivo && Serial2.available()) {
        String color = Serial2.readStringUntil('\n');
        color.trim();
        if (color == "Rojo") {
            contadorRojo++;
        } else if (color == "Verde") {
            contadorVerde++;
        } else if (color == "Azul") {
            contadorAzul++;
        } else {
            contadorDesconocido++;
        }
    }
}

// Página principal
void handleRoot() {
    String estado = clasificadorActivo ? "Activo" : "Inactivo";
    String html = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Clasificador de Colores</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    text-align: center;
                    background: linear-gradient(to right, #4facfe, #00f2fe, #4f63fe, #7b4ffe);
                    color: #fff;
                    margin: 0;
                    padding: 0;
                    text-shadow: 1px 1px 2px pink;
                }
                h1 {
                    margin-top: 20px;
                }
                table {
                    margin: 20px auto;
                    border-collapse: collapse;
                    width: 60%;
                    color: #333;
                    background: #fff;
                    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.5);
                    text-align: center;
                    text-shadow: 0px 0px 0px;
                }
                th, td {
                    border: 1px solid #ddd;
                    padding: 10px;
                }
                th {
                    background-color: #f4f4f4;
                }
                td {
                    background-color: #fff;
                    transition: background-color 0.3s, transform 0.3s;
                }
                td.rojo.highlight {
                    background-color: #ff9999;
                }
                td.verde.highlight {
                    background-color: #99ff99;
                }
                td.azul.highlight {
                    background-color: #9999ff;
                }
                td.highlight {
                    transform: scale(1.1);
                }
                button {
                    padding: 10px 20px;
                    font-size: 16px;
                    margin: 10px;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                    color: #333;
                    background: linear-gradient(to left, #cce6f0, #ccf0e8, #cccff0, #e0ccf0);
                    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.5);
                    transition: background-color 0.3s, transform 0.2s;
                }
                button:hover {
                    background-color: #45a049;
                }
                button:active {
                    transform: scale(0.95);
                }
            </style>
            <script>
                function actualizarTabla() {
                    fetch('/update')
                    .then(response => response.json())
                    .then(data => {
                        actualizarContador('rojo', data.rojo, 'rojo');
                        actualizarContador('verde', data.verde, 'verde');
                        actualizarContador('azul', data.azul, 'azul');
                    });
                }

                function actualizarContador(id, valor, clase) {
                    const td = document.getElementById(id);
                    const previo = parseInt(td.innerText);
                    if (previo !== valor) {
                        td.innerText = valor;
                        td.classList.add(clase, 'highlight');
                        setTimeout(() => {
                            td.classList.remove('highlight');
                        }, 500);
                    }
                }

                function toggleClasificador() {
                    fetch('/toggle')
                    .then(response => response.text())
                    .then(data => {
                        document.getElementById('estado').innerText = data;
                    });
                }

                setInterval(actualizarTabla, 100); // Actualizar cada 200 ms
            </script>
        </head>
        <body>
            <h1>Clasificador de Colores</h1>
            <p>Estado del clasificador: <span id="estado">)rawliteral" +
                  estado +
                  R"rawliteral(</span></p>
            <button onclick="toggleClasificador()">Encender/Apagar Clasificador</button>
            <table>
                <tr><th>Color</th><th>Contador</th></tr>
                <tr><td>Rojo</td><td id="rojo">0</td></tr>
                <tr><td>Verde</td><td id="verde">0</td></tr>
                <tr><td>Azul</td><td id="azul">0</td></tr>
            </table>
        </body>
        </html>
    )rawliteral";
    server.send(200, "text/html", html);
}

// Ruta para obtener datos de la tabla
void handleUpdate() {
    String json = "{";
    json += "\"rojo\":" + String(contadorRojo) + ",";
    json += "\"verde\":" + String(contadorVerde) + ",";
    json += "\"azul\":" + String(contadorAzul) + ",";
    json += "\"desconocido\":" + String(contadorDesconocido);
    json += "}";
    server.send(200, "application/json", json);
}

// Ruta para reiniciar los contadores
void handleReset() {
    contadorRojo = 0;
    contadorVerde = 0;
    contadorAzul = 0;
    contadorDesconocido = 0;
    server.send(200, "text/plain", "Contadores reiniciados");
}

// Ruta para encender/apagar el clasificador
void handleToggle() {
    clasificadorActivo = !clasificadorActivo;
    if (clasificadorActivo) {
        Serial2.write('A'); // Enviar comando al Arduino para activar el clasificador
        server.send(200, "text/plain", "Activo");
    } else {
        Serial2.write('D'); // Enviar comando al Arduino para desactivar el clasificador
        server.send(200, "text/plain", "Inactivo");
    }
}
