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

//#define DEBUG

unsigned long previousMillis = 0;        
const long interval = 15 * 60 * 1000;

// wifi connection

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h> 
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

ESP8266WiFiMulti wifiMulti;  

ESP8266WebServer serverino(80);    

WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive

IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

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


unsigned long intervalNTP = 5 * 60000; // Request NTP time every 5 minutes
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;
uint32_t actualTime = 0;

unsigned long prevActualTime = 0;

bool alreadydone = false;
int lastUpld;

// DHT
#include <DHT.h>
#define DHTTYPE DHT22 
#define DHTPIN 3

DHT dht(DHTPIN, DHTTYPE);

//BMP
#include <SFE_BMP180.h>
#include <Wire.h>

char status;
double P, T, h, t, voltage;

SFE_BMP180 pressure;

// power supply measure for battery monitoring

ADC_MODE(ADC_VCC);


void setup() {
  // start Serial as TX only (GPIO1) to use the RX pin (GPIO3)
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  pinMode(3,INPUT_PULLUP);
  delay(100);

  connect2wifimulti();

  startUDP();

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);
  
  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

  
//  startOTA();

  dht.begin();
  
// sda/scl to 0/2
  Wire.begin(0,2);
  delay(2000);

  // BPM setup
  pressure.begin();

  readSensors();
//  send2server();
}
 
void loop() {
//   ArduinoOTA.handle();
   unsigned long currentMillis = millis();

   if (currentMillis - prevNTP > intervalNTP) { // If a minute has passed since last NTP request
     prevNTP = currentMillis;
     #ifdef DEBUG
       Serial.println("\r\nSending NTP request ...");
     #endif
     sendNTPpacket(timeServerIP);               // Send an NTP request
   }

  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    #ifdef DEBUG
      Serial.print("NTP response:\t");
      Serial.println(timeUNIX);
    #endif
    lastNTPResponse = currentMillis;
  }// else if ((currentMillis - lastNTPResponse) > 3600000) {
//    Serial.println("More than 1 hour since last NTP response. Rebooting.");
//    Serial.flush();
//    ESP.reset();
//  } 

  actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
  #ifdef DEBUG
    if (actualTime != prevActualTime && timeUNIX != 0) { // If a second has passed since last print
      prevActualTime = actualTime;
      Serial.printf("\rUTC time:\t%d:%d:%d   ", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime));
    } 
  #endif
  int ora = getMinutes(actualTime);
  if ((ora==0) || (ora==15) || (ora==30) || (ora==45)){
    if (!alreadydone) {
      readSensors();
      send2server();
      lastUpld = ora;
      alreadydone = true;     
    }
  } else if (alreadydone) alreadydone = false;
  
/*   if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensors();
    send2server();
   }*/
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

    int x = 0;
    while(x < 10){
      delay(1000);
      h = dht.readHumidity();
      t = dht.readTemperature();
      if (!isnan(h) || !isnan(t)) x=10;
      x++;
    }
    
    
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

  voltage = 0;
  for (int i=0; i<30; i++){
  	voltage = voltage + ESP.getVcc();
  }
	voltage = voltage/1024/30;

  #ifdef DEBUG
    Serial.print("temp \t");
    Serial.println(t);
    Serial.print("humi \t");
    Serial.println(h);
    Serial.print("Temp \t");
    Serial.println(T);
    Serial.print("Pres \t");
    Serial.println(P);
    Serial.print("Volt \t");
    Serial.println(voltage);
	
    Serial.println();
  #endif    
}


void send2server(){
  WiFiClient client;
  
  int retries = 25;
  
  while(!!!client.connect(server, 80) && (retries-- > 0)) {
    delay(100);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  #ifdef DEBUG
    Serial.println();
    if(!!!client.connected()) {
       Serial.println("Failed to connect, going back to sleep");
    }
  
  Serial.print("Request resource: "); 
  Serial.println(resource);
  #endif
  client.print(String("GET ") + resource + apiKey + "&field1=" + t + "&field2=" + h + "&field3=" + T + "&field4=" + P + "&field5=" + voltage +
                  " HTTP/1.1\r\n" +
                  "Host: " + server + "\r\n" + 
                  "Connection: close\r\n\r\n");

                  
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
//  if(!!!client.available()) {
//     Serial.println("No response, going back to sleep");
//  }
  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();
  ESP.deepSleep(13*60*1000000);
  delay(2000);

}

void handleRoot() {
  IPAddress ipAddr = WiFi.localIP();
  
  serverino.send(200, "text/html", "<p>" + WiFi.SSID() + "</p><p>" + ipAddr[0] + "." + ipAddr[1] + "." + ipAddr[2] + "." + ipAddr[3] +  "<p>temp: " + String(t) + "</p><p>humi:" + String(h) + "</p><p>Temp: " + String(T) + "</p><p>Pres: " + String(P) + "</p><p>Volt: " + String(voltage) + "</p><p>last " + lastUpld + "</p><p>UTC: " + getHours(actualTime) + ":" + getMinutes(actualTime) + ":" + getSeconds(actualTime) + "</p>");
}

void handleNotFound(){
  serverino.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void startOTA() {
  ArduinoOTA.setHostname("mAteo2");
  ArduinoOTA.setPassword("mAteo2");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}


