#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ThingSpeak.h>

Adafruit_BME280 bme;
WiFiClient client;
WebServer server(80);

#include <secrets.h>

float temperature, humidity, pressure;

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
  ptr += temperature;
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

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  if (!bme.begin(BME280_ADDRESS_ALTERNATE))
  {
    while (1)
    {
      Serial.println("Keinen BME280 Sensor gefunden!");
      delay(1000);
    }
  }

  Serial.println("Connecting to ");
  Serial.println(WIFI_SSID);

  //connect to your local wi-fi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  ThingSpeak.begin(client); // Initialize ThingSpeak
}

unsigned long lastSensorUpdate = 0;
unsigned long lastThingSpeakUpdate = 0;

void loop()
{
  int now = millis();
  if (now - lastSensorUpdate > 5000)
  {
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
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, pressure);

    // set the status
    ThingSpeak.setStatus("Alles in Ordnung!");

    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_KEY);
    if (x == 200)
    {
      Serial.println("Channel update successful.");
    }
    else
    {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastThingSpeakUpdate=now;
  }

  server.handleClient();
}
