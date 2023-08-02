#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define ONE_WIRE_BUS 22 // 4k7 pullup
#define PWM_PIN 3
#define DIR_PIN 12

LiquidCrystal_I2C lcd(0x27,20,4);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

// Update these with values suitable for your hardware/network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xBF };
IPAddress server(192, 168, 1, 100);
IPAddress ip(192, 168, 0, 29);
IPAddress myDns(192, 168, 0, 1);

EthernetClient eClient;
PubSubClient client(eClient);

long lastReconnectAttempt = 0;

float tempF = 0.00;
float LowFanTemp = 70.00;
float HighFanTemp = 80.00;

unsigned long lastTimer = 0UL;


boolean reconnect() {
  if (client.connect("ArduinoRack")) {


  }
  return client.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String tmpTopic = topic;
  char tmpStr[length+1];
  for (int x=0; x<length; x++) {
    tmpStr[x] = (char)payload[x]; // payload is a stream, so we need to chop it by length.
  }
  tmpStr[length] = 0x00; // terminate the char string with a null



}

void setup() {
  client.setServer(server, 1883);
  client.setCallback(callback);

  lcd.init();
  lcd.backlight();

  sensors.begin();
  sensors.getAddress(insideThermometer, 0);
  sensors.setResolution(insideThermometer, 9);


 //Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    //Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      //Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      //errorProc(1);
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      //Serial.println("Ethernet cable is not connected.");
      //errorProc(2);
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    //Serial.print("  DHCP assigned IP ");
    //Serial.println(Ethernet.localIP());
  }
  delay(1500);
  lastReconnectAttempt = 0;

  /*for (int i=30; i<= 46; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH); // Relays are active with a LOW signal
  }*/
  pinMode(PWM_PIN,OUTPUT);
  pinMode(DIR_PIN,OUTPUT);
  analogWrite(PWM_PIN,0);
  digitalWrite(DIR_PIN,LOW);
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }

  if (millis() - lastTimer >= 1000) {

    lastTimer = millis();

    sensors.requestTemperatures();
    float tempC = sensors.getTempC(insideThermometer);
    if(tempC == DEVICE_DISCONNECTED_C)
    {
    //Serial.println("Error: Could not read temperature data");
    return;
    }
    tempF = DallasTemperature::toFahrenheit(tempC);

    int PwmOut = map(tempF,LowFanTemp,HighFanTemp,5,255);
    analogWrite(PWM_PIN,PwmOut);


    char sz[32];
    dtostrf(tempF, 4, 2, sz);
    client.publish("computerrack/tempf",sz);
    sprintf(sz, "%d", PwmOut);
    client.publish("computerrack/fanpwm",sz);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp F: ");
    dtostrf(tempF, 4, 2, sz);
    lcd.print(sz);
    lcd.setCursor(0,1);
    sprintf(sz, "PWM: %d", PwmOut);
    lcd.print(sz);
  }
}
