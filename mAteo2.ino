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

// wifi connection

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>


// networks connections

const char* ssid      = "Linkem2.4GHz_9080AD";
const char* password  = "nostrarete!";
const char* ssid2     = "linkem2.4GHz_9080AD-ext";
const char* password2 = "nostrarete!";

// Replace with your unique Thing Speak WRITE API KEY
const char* apiKey    = "A1QSQK269EGGKM8D";


const char* resource  = "/update?api_key=";
const char* server    = "api.thingspeak.com";


ESP8266WiFiMulti wifiMulti;  
void connect2wifimulti();


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

  dht.begin();
  
// sda/scl to 0/2
  Wire.begin(0,2);
  delay(2000);

  // BPM setup
  pressure.begin();

  readSensors();
  send2server();
  
}

void loop() {
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
  for (int i=0; i<10; i++){
  	voltage = voltage + ESP.getVcc();
    delay(100);
  }
	voltage = voltage/1024/10;

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

  while(client.available()){
    Serial.write(client.read());
  }
  
  Serial.println("\nclosing connection");
  client.stop();
  ESP.deepSleep(29*30*1000000); //sleep for 14min and 30sec
  delay(2000);

}

