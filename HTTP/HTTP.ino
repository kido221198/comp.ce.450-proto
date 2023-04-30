#include <TH02_dev.h>
#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#define ShowSerial Serial
#define COMSerial Serial1
#define ShowSerial SerialUSB


float LIGHT_CONF = 0.0;
int   INTERVAL = 3;
float LOWER_LIGHT = 0.0;
float UPPER_LIGHT = 100.0;

TH02_dev TH02;
WiFiClient wifiClient;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(F(""));
  Serial.println(F("Initializing.."));
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    // don't continue:
    while (true);
  }
  setup_wifi();

  // you're connected now, so print out the data:
  Serial.println(F("You're connected to the network"));
  
  // Initialize the Analog reading
  TH02.begin();
  Serial.println(F("Initializing analog reading"));
  delay(100);
  Serial.flush();
  analogWrite(A1, 255);
}

void loop() {
  // check the network connection once every 10 seconds:

  delay(INTERVAL * 1000);
  if (WiFi.status() != WL_CONNECTED) {setup_wifi();}
  Serial.println(WiFi.status());

  // setup_wifi();
  Serial.println(F("New loop started!"));
  getConfig();

  delay(500);
  float value = humidVal();
  Serial.print(F("Humidity [%]: "));
  Serial.println(value);
  postRequest("humidity", value);

  delay(500);
  value = tempVal();
  Serial.print(F("Temperature [oC]: "));
  Serial.println(value);
  postRequest("temperature", value);

  delay(500);
  value = lightVal();
  Serial.print(F("Light [%]: "));
  Serial.println(value);
  if (value > 100) {value = 100;}
  postRequest("light", value);

  if (value > UPPER_LIGHT) {fader(0);} 
  else if (value < LOWER_LIGHT) {fader(100);} 
  else {fader(LIGHT_CONF);}

  Serial.flush();
  
}


void setup_wifi() {
  char SECRET_SSID[] = "Puck";
  char SECRET_PASS[] = "mp070598";
  // WiFi.disconnect();
  delay(1000);

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(SECRET_SSID);

  WiFi.begin(SECRET_SSID, SECRET_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 5) {
    Serial.print(F("."));
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    i++;
  }
  if (i < 5) {
    Serial.println(F("WiFi connected"));
  }
  else {
    Serial.println(F("WiFi not connected"));
  }
  
  Serial.flush();
}

void getConfig() {
  delay(500);
  int value = getRequest("c_light");
  if (value > 0) {
    Serial.print(F("Control light [%]: "));
    Serial.println(value);
    LIGHT_CONF = value;
  }

  delay(500);
  value = getRequest("interval");
  if (value > 0) {
    INTERVAL = value;
    Serial.print(F("Interval [sec]: "));
    Serial.println(value);
  }

  delay(500);
  value = getRequest("c_upper_light");
  if (value > 0 && value > LOWER_LIGHT) {
    UPPER_LIGHT = value;
    Serial.print(F("Upper Light Threshold [%]: "));
    Serial.println(value);
  }

  delay(500);
  value = getRequest("c_lower_light");
  if (value > 0 && value < UPPER_LIGHT) {
    LOWER_LIGHT = value;
    Serial.print(F("Lower Light Threshold [%]: "));
    Serial.println(value);
  }

}

void postRequest(String label, float value) {
  // if (WiFi.status() != WL_CONNECTED) {setup_wifi();}

  char  HOST_NAME[] = "industrial.api.ubidots.com";
  int   HTTP_PORT = 80;
  Serial.print(F("Starting to connect to server.."));
  Serial.flush();

  StaticJsonBuffer<100> jb;
  JsonObject& doc = jb.createObject();
  doc[label] = value;

  if (wifiClient.connect(HOST_NAME, HTTP_PORT)) {
    Serial.println(F("connected!"));
    
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
        Serial.print(c);
      }
    }
    wifiClient.stop();
    Serial.println(F("Connection closed."));

  } else {
    Serial.println(F("failed!"));
  }
}


int getRequest(String label) {
  // if (WiFi.status() != WL_CONNECTED) {setup_wifi();}

  char  HOST_NAME[] = "industrial.api.ubidots.com";
  int   HTTP_PORT = 80;

  Serial.print(F("Starting to connect to server.."));
  Serial.flush();

  if (wifiClient.connect(HOST_NAME, HTTP_PORT)) {
    Serial.println(F("connected!"));

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
        // read an incoming byte from the server and print it to serial monitor:
        char x = wifiClient.read();
        Serial.print(x);
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

    Serial.print(F("Received "));
    Serial.print(label);
    Serial.print(F(" as "));
    Serial.print(result);
    Serial.println(F("."));

    return result;

  } else {
    Serial.println(F("failed!"));
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