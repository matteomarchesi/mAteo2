/*
 * #mAteo2
 * weather station with ESP-01
 *
 * GPIO0/2 used as SCA/SCL for BMP180
 * GPIO3 (RX) used as data pin of DHT22
 *
 * ****WARNING*****
 * TO FLASH NEW CODE
 * 1- Disconnect GPIO3 from DHT22 data pin
 * 2- ground GPIO0 (no need to disconnect from BMP180)
 *
 * BOM:
 * - ESP-01
 * - DHT22
 * - BMP180
 * - 330R resistor (1x)
 * - 3K3 resistors (4x)
 * - USB programmer interface (for programming/debugging only)
 * to be added:
 * - power supply
 *   . 9V battery (check to be evaluated)
 *   . LM1117-3.3 voltage regulator
 *   . 10uF tantalum cap for voltage regulator
 * - programming switcher
 *   . slide switch for easy flashing
 *   OR
 *   . test pins (2x 3pin strips) + 2x jumpers
 * - test pins for USB programmer (1x 6 pin strip)
 * to be evalueted:
 * - usage of lipo battery and charger
 * 
 *  ESP-01 pinout
 *
 *     A    RX     o o  Vcc
 *     N    GPIO0  o o  RST
 *     T    GPIO2  o o  CH_PD
 *          GND    o o  TX
 *
 * ESP-01 connections
 *     programmer TX 	330R  (a) 	o o  		Vcc
 *     Vcc     			3K3   (b)	o o  3K3	Vcc
 *     Vcc     			3K3   (c) 	o o  3K3	Vcc
 *     Gnd     						o o  		programmer TR
 *
 *	Programmer
 *	TX to ESP-01 RX
 *  RX to 330R to ESP-01 TX
 *  Vcc to Vcc
 *  Gnd to GND
 *
 *	DHT22
 *  Sig to (a) ESP-01
 *  Vcc to Vcc
 *  Gnd to GND
 *
 *	BMP180
 *	SDA to (b) ESP-01
 *  SCL to (c) ESP-01
 *  Vcc to Vcc
 *  Gnd to GND
 *
 */

unsigned long previousMillis = 0;        
const long interval = 15 * 60 * 1000;

// wifi connection

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h> 
#include <ESP8266WebServer.h>

ESP8266WiFiMulti wifiMulti;  

ESP8266WebServer serverino(80);    

void handleRoot();              
void handleNotFound();

const char* ssid      = "Linkem2.4GHz_9080AD";
const char* password  = "nostrarete!";
const char* ssid2     = "linkem2.4GHz_9080AD-ext";
const char* password2 = "nostrarete!";


// Replace with your unique Thing Speak WRITE API KEY
const char* apiKey    = "A1QSQK269EGGKM8D";
const char* resource  = "/update?api_key=";

// Thing Speak API server 
const char* server    = "api.thingspeak.com";


// DHT
#include <DHT.h>
#define DHTTYPE DHT22 
#define DHTPIN 3

DHT dht(DHTPIN, DHTTYPE);

//BMP
#include <SFE_BMP180.h>
#include <Wire.h>

char status;
double P, T, h, t;

SFE_BMP180 pressure;


void setup() {
  delay(5000);
  // start Serial as TX only (GPIO1) to use the RX pin (GPIO3)
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  pinMode(3,INPUT_PULLUP);
  delay(10);

  connect2wifimulti();
  
// sda/scl to 0/2
  Wire.begin(0,2);
  delay(500);

  // BPM setup
  pressure.begin();
  
}
 
void loop() {
   unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensors();
    send2server();
   }
   serverino.handleClient(); 
}

void connect2wifimulti(){
  
  wifiMulti.addAP(ssid, password);   
  wifiMulti.addAP(ssid2, password2);

  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(1000);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); 
  Serial.println('\n');

  if (!MDNS.begin("mAteo2")) { 
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  
  serverino.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  serverino.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  serverino.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void connect2wifi(){
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
   
}


void readSensors(){
    h = dht.readHumidity();
    t = dht.readTemperature();
    
    status = pressure.startTemperature();
    if (status != 0)
    {
      // Wait for the measurement to complete:
      delay(status);
      status = pressure.getTemperature(T);
    }
  
    status = pressure.startPressure(3);
    if (status != 0)
    {
      // Wait for the measurement to complete:
      delay(status);
      status = pressure.getPressure(P,T);
    }
    Serial.print("temp \t");
    Serial.println(t);
    Serial.print("humi \t");
    Serial.println(h);
    Serial.print("Temp \t");
    Serial.println(T);
    Serial.print("Pres \t");
    Serial.println(P);
    Serial.println();
    
}


void send2server(){
  WiFiClient client;
  
  int retries = 25;
  
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if(!!!client.connected()) {
     Serial.println("Failed to connect, going back to sleep");
  }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);
  client.print(String("GET ") + resource + apiKey + "&field1=" + t + "&field2=" + h + "&field3=" + T + "&field4=" + P +
                  " HTTP/1.1\r\n" +
                  "Host: " + server + "\r\n" + 
                  "Connection: close\r\n\r\n");

                  
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
     Serial.println("No response, going back to sleep");
  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();

}

void handleRoot() {
                         
  serverino.send(200, "text/html", "<p>" + WiFi.SSID() + "</p><p>" + WiFi.localIP() +  "<p>temp: " + String(t) + "</p><p>humi:" + String(h) + "</p><p>Temp: " + String(T) + "</p><p>Pres: " + String(P) + "</p>");
}

void handleNotFound(){
  serverino.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
