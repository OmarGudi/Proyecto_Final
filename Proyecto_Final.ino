#include <Wire.h>
#include <XNODE.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <tago.h>

#define CS_PIN 25
#define TIME_BEFORE_CHECKING 5000 // en ms
#define TIME_ON 10000  // Tiempo que la bomba permanecerá encendida (en ms)

const char* ssid = "DMDHouse_2.4Gnormal";
const char* password = "OsoLeonGato69";
const char* tagoToken = "5f9deb34-90ae-41c3-9cf9-635ceffaa56c";
const char* tagoUrl = "https://api.tago.io/data?variable=";
char bombStateChar[2];
tago tag(const_cast<char*>(ssid), const_cast<char*>(password), const_cast<char*>(tagoToken));
XNODE xnode(&Serial2);
bool bomb_state = false;
unsigned long startTime = 0;  // Para controlar el tiempo de encendido

void setup() 
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    Wire.begin();

    if (!tag.connectWifi()) 
    {
        Serial.println("Error: No se pudo conectar al WiFi");
        xnode.SendCommandWithRange("007A", "1", 255, 0, 0);
        xnode.SendCommandWithRange("007A", "2", 255, 0, 0);
        while (1);
    }

    Serial.println("WiFi conectado");
    bomb_state = getButtonState(); // Obtener estado inicial

    if (bomb_state) 
    {
        Serial.println("Bomba ya estaba encendida, manteniendo estado.");
        startTime = millis(); // Guardar tiempo de inicio
        encenderBomba();
    }
    else 
    {
        apagarBomba();
    }
}

void loop() 
{
    bool newState = getButtonState();

    if (newState && !bomb_state) // Si el usuario enciende la bomba
    {
        Serial.println("Encendiendo la bomba...");
        bomb_state = true;
        startTime = millis(); // Guardar el tiempo de inicio
        encenderBomba();
    }
    // Si la bomba está encendida y ha pasado el tiempo definido, apagarla
    if (bomb_state && (millis() - startTime >= TIME_ON)) 
    {
        Serial.println("Tiempo agotado. Apagando la bomba...");
        bomb_state = false;
        apagarBomba();
    }

    // Enviar estado a TAGOIO
    sprintf(bombStateChar, "%d", bomb_state);
    xnode.SendCommand("XN13A", "S", bombStateChar);
    tag.sendData("Bomb_State", String(bomb_state));

    delay(TIME_BEFORE_CHECKING); // Esperar antes de volver a comprobar
}

bool getButtonState() 
{
    if (WiFi.status() == WL_CONNECTED) 
    {
        HTTPClient http;
        String url = String(tagoUrl) + "bomb_state&qty=1"; // Obtener el valor más reciente
        Serial.println("Requesting: " + url);
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Device-Token", tagoToken);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) 
        {
            String payload = http.getString();
            Serial.println("Response from TAGOIO: " + payload);
            DynamicJsonDocument doc(4096);
            deserializeJson(doc, payload);
            if (doc.containsKey("result") && doc["result"].size() > 0) 
            {
                JsonObject latestData = doc["result"][0];
                String value = latestData["value"].as<String>();
                Serial.print("Latest Value: ");
                Serial.println(value);

                if (value == "On") return true;
                if (value == "Off") return false;
                int intValue = latestData["value"].as<int>();
                return (intValue == 1);
            }
        } 
        else 
        {
            Serial.println("HTTP Request Failed! Keeping previous state.");
        }
        http.end();
    }
    return bomb_state; // Devolver estado anterior si falla la solicitud
}

void encenderBomba() 
{
    xnode.SendCommandWithRange("007A", "1", 0, 255, 0);
    xnode.SendCommandWithRange("007A", "2", 0, 255, 0);
    Serial.println("Bomba ENCENDIDA");
}

void apagarBomba() 
{
    xnode.SendCommandWithRange("007A", "1", 255, 0, 0);
    xnode.SendCommandWithRange("007A", "2", 255, 0, 0);
    Serial.println("Bomba APAGADA");
}