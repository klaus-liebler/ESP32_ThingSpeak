//Einbindung aller notwendigen Bibliotheken
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ThingSpeak.h>
#include <secrets.h>

//Kommunikationsobjekt für den Temperatur-Luftfeuchtigkeits-Luftdruck-Modul
Adafruit_BME280 bme;

//Kommunikationsobjekt für WLAN
WiFiClient client;

//Kommunikationsobjekt Webserver
WebServer server(80);

//"Datenmodell" durch einfache globale Variablen
float temperature, humidity, pressure;

//Funktion erzeugt dynamisches HTML - letztlich die Website, die darzustellen ist
void handle_OnConnect()
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 Weather Station</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP32 Weather Station</h1>\n";
  ptr += "<p>Temperature: ";
  ptr += temperature; //<<<<<================Hier wird zum Beispiel die aktuelle Temperatur dynamische ins HTML eingefügt
  ptr += "&deg;C</p>";
  ptr += "<p>Humidity: ";
  ptr += humidity;
  ptr += "%</p>";
  ptr += "<p>Pressure: ";
  ptr += pressure;
  ptr += "hPa</p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  server.send(200, "text/html", ptr);
}

//Generiert eine einfache Fehlerseite
void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

//Funktion wird automatisch vom Framework einmalig beim einschalten bzw nach Reset aufgerufen
void setup()
{
  //Richtet serielle Kommunikationsschnittstelle ein, damit die ganzen Meldungen am PC angezeigt werden können
  Serial.begin(115200);
  delay(100);

  //BME280-Sensor wird eingerichtet
  if (!bme.begin(BME280_ADDRESS_ALTERNATE))
  {
    while (1)
    {
      Serial.println("Keinen BME280 Sensor gefunden!");
      delay(1000);
    }
  }

  //VErbindung zum WIFI wird aufgebaut und mit vielen Ausgaben auf der PC-Seite dokumentiert
  Serial.println("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  //Es wird hinterlegt, welche Funktionen aufzurufen sind, wenn ein Browser sich verbindet
  //Wenn die "Hauptseite", also einfach "/" aufgerufen wird, dann soll handle_OnConnect aufgerufen werden
  server.on("/", handle_OnConnect);
  //wenn etwas anderes aufgerufen wird, dann soll eine einfache Fehlerseite dargestellt werden
  server.onNotFound(handle_NotFound);

  //Ab der nächsten Zeile ist der ESP32 für einen Webbrowser erreichbar, weil der sog "HTTP-Server" gestartet wird
  server.begin();
  Serial.println("HTTP server started");

  //Die Verbindung zu ThingSpeak wird aufgebaut
  ThingSpeak.begin(client);
}

unsigned long lastSensorUpdate = 0;
unsigned long lastThingSpeakUpdate = 0;

//Diese Schleife wird ständig immer und immer wieder aufgerufen
void loop()
{
  //Hole die aktuelle Zeit
  int now = millis();

  if (now - lastSensorUpdate > 5000)
  {
    //Wenn seit dem letzten Sensor-Update mehr als 5sek vergangen sind: Führe ein Sensor-Update durch (also: Hole die aktuellen Messwerte vom Sensor)
    //und lege diese im "Datenmodell" ab. Gebe die aktuellen Werte auch aus.
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;
    Serial.print("Temperatur = ");
    Serial.print(temperature);
    Serial.println(" *C");
    Serial.print("Feuchtigkeit = ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.print("Luftdruck = ");
    Serial.print(pressure);
    Serial.println(" hPa");
    lastSensorUpdate = now;
  }
  if (now - lastThingSpeakUpdate > 20000)
  {
    //Wenn seit dem letzten ThingSpeak-Update mehr als 20sek vergangen sind: Sende aktuelle Messwerte an ThingSpeak
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, pressure);
    ThingSpeak.setStatus("Alles in Ordnung!");
    int x = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_KEY);
    if (x == 200)
    {
      Serial.println("Aktualisierung erfolgreich.");
    }
    else
    {
      Serial.println("Problem bei der Aktualisierung des Kanals. HTTP error code " + String(x));
    }
    lastThingSpeakUpdate=now;
  }
  //Falls ein Browser eine Verbindung aufbaut: Höre, was er will und liefere die richtige Seite an ihn aus.
  server.handleClient();
}
