// wifi connection

#include <ESP8266WiFi.h>
 
const char* ssid     = "Linkem2.4GHz_9080AD";
const char* password = "nostrarete!";

// wifi server
WiFiServer server(80);

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

  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  pinMode(3,INPUT_PULLUP);
  delay(10);
 
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
   
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

// per DHT
//  pinMode(2,INPUT);

// sda/scl to 0/2
  Wire.begin(0,2);
  delay(500);
// BPM setup
  pressure.begin();
  
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  delay(2000);
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
   
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<head>");
  client.println("<style>");
  client.println("html{box-sizing:border-box}*,*:before,*:after{box-sizing:inherit}");
  client.println("html,body{font-family:Verdana,sans-serif;font-size:15px;line-height:1.5}html{overflow-x:hidden}");
  client.println("h1{font-size:36px}.w3-serif{font-family:serif}");
  client.println("h1{font-family:\"Segoe UI\",Arial,sans-serif;font-weight:400;margin:10px 0;text-align: center;}.w3-wide{letter-spacing:4px}");
  client.println(".w3-table{border-collapse:collapse;border-spacing:0;width:100%;display:table}");
  client.println(".w3-striped tbody tr:nth-child(even){background-color:#f1f1f1}");
  client.println(".w3-table td,.w3-table th{padding:8px 8px;display:table-cell;text-align:left;vertical-align:top}");
  client.println(".w3-table th:first-child,.w3-table td:first-child{padding-left:16px}");
  client.println(".w3-teal{color:#fff!important;background-color:#009688!important}");
  client.println("</style>");
  client.println("</head>");
  
  client.println("<body>");
  client.println("<div class=\"w3-container w3-teal\">");
  client.println("<h1>mAteo 2.0</h1>");
  client.println("</div>");
  client.println("<div class=\"w3-row-padding\">");
  client.println("  <table class=\"w3-table w3-striped\">");
  client.println("  <tr>");
  client.println("    <th>Dato</th>");
  client.println("    <th>Valore</th>");
//  client.println("    <th>Aggiornamento</th>");
  client.println("  </tr>");
  client.println("  <tr>");
  client.println("    <td>Temperatura (DHT)</td>");
  client.println("    <td>");
  client.println(t);
  client.println("  &#8451;</td>");
//  client.println("    <td></td>");
  client.println("  </tr>");
  
  client.println("  <tr>");
  client.println("    <td>Umidita`</td>");
  client.println("    <td>");
  client.println(h);
  client.println("  %RH</td>");
//  client.println("    <td></td>");
  client.println("  </tr>");

  client.println("  <tr>");
  client.println("    <td>Pressione</td>");
  client.println("    <td>");
  client.println(P);
  client.println("  mb</td>");
 // client.println("    <td></td>");
  client.println("  </tr>");

  client.println("  <tr>");
  client.println("    <td>Temperatura (BMP)</td>");
  client.println("    <td>");
  client.println(T);
  client.println("  &#8451;</td>");
//  client.println("    <td></td>");
  client.println("  </tr>");
  
  client.println("    </table>");
  client.println("</div>");
  client.println("</body>");


  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}

