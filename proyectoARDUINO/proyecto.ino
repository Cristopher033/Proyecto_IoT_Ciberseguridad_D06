#include <Adafruit_TCS34725.h>
#include <AccelStepper.h>

// Configuración del sensor de color TCS34725
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);

// Configuración del motor paso a paso
#define motorInterfaceType 4 // Define el tipo de interfaz (4 hilos)
AccelStepper myStepper(motorInterfaceType, 8, 10, 9, 11);

// Variables globales
bool clasificadorActivo = false;
bool motorMoviendose = false;
unsigned long ultimaLectura = 0; // Control de tiempo para lecturas
const unsigned long intervaloLectura = 50; // Intervalo entre lecturas (ms)

// Posiciones del motor para cada color
const int posicionRojo = 1400;
const int posicionVerde = 1600;
const int posicionAzul = 1800;
//const int posicionInicial = 0;

void setup() {
    Serial.begin(9600); // Comunicación serial

    // Configurar motor
    myStepper.setMaxSpeed(1500); // Velocidad máxima del motor
    myStepper.setAcceleration(1000); // Aceleración del motor

    // Inicializar sensor de color
    if (tcs.begin()) {
        Serial.println("Sensor de color detectado.");
    } else {
        Serial.println("Error: No se pudo encontrar el sensor de color.");
        while (true); // Detener ejecución si no se encuentra el sensor
    }

    Serial.println("Arduino listo para recibir comandos.");
}

void loop() {
    // Revisar comandos seriales
    if (Serial.available()) {
        char comando = Serial.read(); // Leer comando del ESP32
        switch (comando) {
            case 'A': // Activar clasificador
                clasificadorActivo = true;
                Serial.println("Comando recibido: Activar clasificador");
                break;

            case 'D': // Desactivar clasificador
                clasificadorActivo = false;
                Serial.println("Comando recibido: Desactivar clasificador");
                break;

            default:
                Serial.println("Comando no reconocido.");
        }
    }

    // Ejecutar clasificador si está activo
    if (clasificadorActivo && !motorMoviendose && millis() - ultimaLectura >= intervaloLectura) {
        ultimaLectura = millis();

        // Leer datos del sensor
        uint16_t r, g, b, c;
        tcs.getRawData(&r, &g, &b, &c);

        // Detección rápida de color
        String color = detectarColor(r, g, b);
        Serial.println(color); // Enviar color al ESP32

        // Clasificar el objeto
        clasificarObjeto(color);
    }

    // Mover el motor si hay un objetivo pendiente
    if (motorMoviendose) {
        if (myStepper.run() == false) { 
            // Detener motor una vez alcanzada la posición
            motorMoviendose = false;
            Serial.println("Movimiento completado.");
        }
    }
}

// Función para determinar el color
String detectarColor(uint16_t r, uint16_t g, uint16_t b) {
    if ((r > g + 50) && (r > b + 50)) {
        return "Rojo";
    } else if ((g > r + 30) && (g > b + 30)) {
        return "Verde";
    } else if ((b > r + 40) && (b > g + 40)) {
        return "Azul";
    } else {
        return "Desconocido";
    }
}

// Función para clasificar el objeto según su color
void clasificarObjeto(String color) {
    int objetivo;

    if (color == "Rojo") {
        objetivo = posicionRojo;
        Serial.println("Objeto clasificado en el contenedor rojo.");
    } else if (color == "Verde") {
        objetivo = posicionVerde;
        Serial.println("Objeto clasificado en el contenedor verde.");
    } else if (color == "Azul") {
        objetivo = posicionAzul;
        Serial.println("Objeto clasificado en el contenedor azul.");
    } else {
        Serial.println("Color no reconocido.");
    }

    // Configurar nueva posición del motor
    myStepper.moveTo(objetivo);
    motorMoviendose = true;
}