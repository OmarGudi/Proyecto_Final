#define BLYNK_TEMPLATE_NAME "Bomba de Agua"
#define BLYNK_TEMPLATE_ID "TMPL2OqOkP38A"
#define BLYNK_AUTH_TOKEN "EFN2N_SbqQkSmFPgG7WNMeZW623vBaUX"

#include <Wire.h>
#include <XNODE.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define CS_PIN 25
#define TIME_BEFORE_CHECKING 5000 // en ms
#define TIME_ON 10000  // Tiempo que la bomba permanecerá encendida (en ms)

// Red Wi-Fi
const char* ssid = "DMDHouse_2.4Gnormal";
const char* password = "OsoLeonGato69";

// Pines virtuales de Blynk
#define VIRTUAL_BOMB_BUTTON V0
#define VIRTUAL_BOMB_STATE V1

XNODE xnode(&Serial2);
bool bomb_state = false;
unsigned long startTime = 0;

// Función que se ejecuta cuando se presiona el botón en la app
BLYNK_WRITE(VIRTUAL_BOMB_BUTTON) {
  int pinValue = param.asInt();
  if (pinValue == 1 && !bomb_state) {
    bomb_state = true;
    startTime = millis();
    encenderBomba();
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  Wire.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    xnode.SendCommandWithRange("007A", "1", 255, 0, 0);
    xnode.SendCommandWithRange("007A", "2", 255, 0, 0);
  }

  Serial.println("\nWiFi conectado");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Estado inicial
  if (bomb_state) {
    startTime = millis();
    encenderBomba();
  } else {
    apagarBomba();
  }
}

void loop() {
  Blynk.run();

  // Si la bomba está encendida y se supera el tiempo límite, apagarla
  if (bomb_state && (millis() - startTime >= TIME_ON)) {
    bomb_state = false;
    apagarBomba();
    Blynk.virtualWrite(VIRTUAL_BOMB_BUTTON, 0);  // Apagar botón en app
  }

  // Enviar estado actual a la app
  Blynk.virtualWrite(VIRTUAL_BOMB_STATE, bomb_state);
  char nodeId[] = "XN13A";
  char command[] = "S";
  char value[2];
  sprintf(value, "%d", bomb_state);
  xnode.SendCommand(nodeId, command, value);


  delay(TIME_BEFORE_CHECKING);
}

void encenderBomba() {
  //Muestra de forma visual que la bomba esta encendida usando el led.
  xnode.SendCommandWithRange("007A", "1", 0, 255, 0);
  xnode.SendCommandWithRange("007A", "2", 0, 255, 0);
  Serial.println("Bomba ENCENDIDA");
}

void apagarBomba() {
  //Muestra de forma visual que la bomba esta apagada usando el led.
  xnode.SendCommandWithRange("007A", "1", 255, 0, 0);
  xnode.SendCommandWithRange("007A", "2", 255, 0, 0);
  Serial.println("Bomba APAGADA");
}