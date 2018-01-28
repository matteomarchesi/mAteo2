# mAteo2

extra small "weather station" build with ESP8266 ESP-01

Part list:
- ESP-01
- BMP180
- DHT22
- 4x 3K3 resistors
- 1x 330R resistor
- serial programmer (to be removed)

Connections:

ESP-01 GPIO0/GPIO2 used as SCA/SCL to BMP180
ESP-01 RX (GPIO3) to DHT22

When flashing:
GPIO0 to be connected to GND
RX pin to be disconnected from DHT22

on serial monitor you'll see the IP address to be used
