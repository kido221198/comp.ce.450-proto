#include <TH02_dev.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
// #define ShowSerial Serial
#define COMSerial Serial1
// #define ShowSerial SerialUSB


float LIGHT_CONF = 0.0;
int   INTERVAL = 3;
float LOWER_LIGHT = 0.0;
float UPPER_LIGHT = 100.0;

TH02_dev TH02;
WiFiClient wifiClient;

void setup() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    // don't continue:
    while (true);
  }
  setup_wifi();
  
  // Initialize the Analog reading
  TH02.begin();
  delay(100);
  analogWrite(A1, 255);
}

void loop() {
  // check the network connection once every 10 seconds:
  // char SECRET_SSID[] = "Puck";
  // char SECRET_PASS[] = "mp070598";

  delay(INTERVAL * 1000);
  // if (WiFi.status() != WL_CONNECTED) {
  // setup_wifi();
  // }
  getConfig();

  delay(500);
  float value = humidVal();
  postRequest("humidity", value);

  delay(500);
  value = tempVal();
  postRequest("temperature", value);

  delay(500);
  value = lightVal();
  if (value > 100) {value = 100;}
  postRequest("light", value);

  if (value > UPPER_LIGHT) {fader(0);} 
  else if (value < LOWER_LIGHT) {fader(100);} 
  else {fader(LIGHT_CONF);}

}


void setup_wifi() {
  char SECRET_SSID[] = "Puck";
  char SECRET_PASS[] = "mp070598";

  delay(10);
  // We start by connecting to a WiFi network
  // WiFi.begin(SECRET_SSID, SECRET_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 3) {
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    i++;
  }
}

void getConfig() {
  delay(500);
  int value = getRequest("c_light");
  if (value > 0) {
    LIGHT_CONF = value;
  }

  // delay(500);
  // value = getRequest("interval");
  // if (value > 0) {
  //   INTERVAL = value;
  // }

  delay(500);
  value = getRequest("c_upper_light");
  if (value > 0 && value > LOWER_LIGHT) {
    UPPER_LIGHT = value;
  }

  delay(500);
  value = getRequest("c_lower_light");
  if (value > 0 && value < UPPER_LIGHT) {
    LOWER_LIGHT = value;
  }

}

void postRequest(String label, float value) {
  // if (WiFi.status() != WL_CONNECTED) {setup_wifi();}

  char  HOST_NAME[] = "industrial.api.ubidots.com";
  int   HTTP_PORT = 80;

  StaticJsonBuffer<100> jb;
  JsonObject& doc = jb.createObject();
  doc[label] = value;

  if (wifiClient.connect(HOST_NAME, HTTP_PORT)) {
    
    wifiClient.println("POST /api/v1.6/devices/plant01/ HTTP/1.1");
    wifiClient.println("Host: industrial.api.ubidots.com");
    wifiClient.println("X-Auth-Token: BBFF-tHRajzKOSLXIzPeskn4xfNSTzcX7pQ");
    wifiClient.print("Content-Length: ");
    wifiClient.println(doc.measureLength());
    wifiClient.println("Content-Type: application/json");
    wifiClient.println("Connection: close");
    wifiClient.println(); // end HTTP header

    doc.printTo(wifiClient);
    wifiClient.println();
        
    while (wifiClient.connected()) {
      if (wifiClient.available()) {
        char c = wifiClient.read();
      }
    }
    wifiClient.stop();

  } else {}
}


int getRequest(String label) {
  // if (WiFi.status() != WL_CONNECTED) {setup_wifi();}

  char  HOST_NAME[] = "industrial.api.ubidots.com";
  int   HTTP_PORT = 80;

  if (wifiClient.connect(HOST_NAME, HTTP_PORT)) {
    wifiClient.print("GET /api/v1.6/devices/plant01/");
    wifiClient.print(label);
    wifiClient.println("/lv HTTP/1.1");
    wifiClient.println("Host: industrial.api.ubidots.com");
    wifiClient.println("X-Auth-Token: BBFF-tHRajzKOSLXIzPeskn4xfNSTzcX7pQ");
    wifiClient.println("Connection: close");
    wifiClient.println(); // end HTTP header

    // send HTTP body
    wifiClient.println("");
    int decimal_count = 0;
    char c[3];
    
    while (wifiClient.connected()) {
      if (wifiClient.available()){

        char x = wifiClient.read();
        if (x == '.') {
          decimal_count++;
          if (decimal_count == 2) {break;}
        }
        c[0] = c[1];
        c[1] = c[2];
        c[2] = x;

      }
    }

    wifiClient.stop();

    int c0 = c[0]-'0';
    int c1 = c[1]-'0';
    int c2 = c[2]-'0';
    int result;

    if (c0 < 0) {result = c1 * 10 + c2;} 
    else {result = c0 * 100 + c1 * 10 + c2;}
    return result;

  } else {
    return -1;
  }
}

void fader (int amount) { //give the pin and the pwm percentage value.
  int pwm = map(amount, 0, 100, 0, 255);
  analogWrite(5, pwm);
}

float tempVal(){
  return TH02.ReadTemperature();
}

float humidVal(){
  return TH02.ReadHumidity();
}

int lightVal(){
  int value = analogRead(A0);
  value = map(value, 0, 440, 0, 100);
  return value;
}