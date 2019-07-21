#include <Wire.h>
#include <Adafruit_BMP085.h>

#include <Adafruit_CC3000.h>
#include <SPI.h>

// network
#define WLAN_SSID "Ann_plus"
#define WLAN_PASS "24216617"
#define WLAN_SECURITY WLAN_SEC_WPA2
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
#define LISTEN_PORT           888    // What TCP port to listen on for connections.
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2); // you can change this clock speed
Adafruit_CC3000_Server chatServer(LISTEN_PORT);

// DHT11
#include "DHT.h"
#define DHTPIN 22
#define DHTTYPE DHT11

// rain sensor
#define IS_NOT_RAINING_PIN 24
#define DRYNESS_PIN A0
#define WETNESS_THRESHOLD 10

// Pressure BMP180
Adafruit_BMP085 bmp;

// DHT 11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(9600);
  if (!(initBMP180())) {
    while (1) {}; // failed to initialize BMP180
  }
  dht.begin();
  initRainSensor(IS_NOT_RAINING_PIN, DRYNESS_PIN);

    Serial.println(F("\nInitializing cc3000..."));
  if (!cc3000.begin()) {
    while (1); // Couldn't begin()! Check your wiring?
  }

    Serial.print(F("\nAttempting to connect to "));
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    while (1); // failed to connect.
  }

  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  chatServer.begin(); // Start listening for connections

  Serial.println("Listening...");
  // to denote it is ready to be connected by socket.
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  // Try to get a connecting client.
  Adafruit_CC3000_ClientRef client = chatServer.available();
  if (client) {
    // Check if there is data available to read.
    if (client.available() > 0) { // if receiving a string, it will be provoked the times of the lengths of the string.
      // it has to be read. otherwise, it will be provoked multiple times. bug. I guess "read" method will pop char from buffer.
      uint8_t ch = client.read(); // use char(ch) to convert to char.

      if (!(ch == '\n')){ // client has to send \n to act the weather server. otherwise, it crashes this server.
        digitalWrite(LED_BUILTIN, HIGH);
        goto startAgain; // return;
      }
      
      int humidityVal = (int)dht.readHumidity();
      if (isnan(humidityVal)) { // Failed to load humidity
        digitalWrite(LED_BUILTIN, HIGH);
        goto startAgain; // return;
      }

      float tempVal = bmp.readTemperature();
      float pressureVal = (bmp.readPressure() / 100);

      String dryValue = String(dryness(DRYNESS_PIN));
      String notRaining = String(isNotRaining(IS_NOT_RAINING_PIN));
      String temp = String(tempVal);
      String humidity = String(humidityVal);
      String pressure = String(pressureVal);

      printlnToClient(&chatServer, &temp);
      printlnToClient(&chatServer, &humidity);
      printlnToClient(&chatServer, &pressure);
      printlnToClient(&chatServer, &dryValue);
      printlnToClient(&chatServer, &notRaining);

      chatServer.write("\r");
    }
  }
  startAgain:;
}
/*---------------------------------------------------------------------------------------*/
void printlnToClient(Adafruit_CC3000_Server* server, String* msg) {
  char charBuff[40] = {0};
  (*msg).toCharArray(charBuff, sizeof(charBuff));
  (*server).write(charBuff);
}

bool initBMP180() {
  if (bmp.begin()) {
    return true;
  } else {
    return false;
  }
}

bool isNotRaining(int isRainPinD) {
  return digitalRead(isRainPinD);
}

void initRainSensor(int isNotRainingPin, int howDryPin) {
  pinMode(isNotRainingPin, INPUT);
  pinMode(howDryPin, INPUT);
}

int dryness(int howDryPin) {
  return map(analogRead(howDryPin), 0, 1023, 0, WETNESS_THRESHOLD);
}
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
        Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
        Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
        Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
        Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
        Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
        Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
        Serial.println();
    return true;
  }
}
/*---------------------------------------------------------------------------------------*/
