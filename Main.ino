#define TINY_GSM_MODEM_SIM800

#define SIM800L_RX     26
#define SIM800L_TX     27
#define SIM800L_PWRKEY 4
#define SIM800L_RST    5
#define SIM800L_POWER  23

int counter = 0;
//Increase RX buffer
#define TINY_GSM_RX_BUFFER 256

#include <TinyGPS++.h> //https://github.com/mikalhart/TinyGPSPlus
#include <TinyGsmClient.h> //https://github.com/vshymanskyy/TinyGSM
#include <ArduinoHttpClient.h> //https://github.com/arduino-libraries/ArduinoHttpClient

const char FIREBASE_HOST[]  = "esp32-sensor-c9a4b-default-rtdb.firebaseio.com"; //firebase link
const String FIREBASE_AUTH  = "qpckoKRqew11RGWv6ijDf6BFTok3xG5N5c4GcEGj"; //AUTH token
const String FIREBASE_PATH  = "/";
const int SSL_PORT          = 443;

char apn[]  = "portalnmms"; //apn setting for vodafone 4g-India
char user[] = "";
char pass[] = "";



HardwareSerial sim800(1);
TinyGsm modem(sim800);

#define RXD2 32
#define TXD2 33
HardwareSerial neogps(2);
TinyGPSPlus gps;

TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);

unsigned long previousMillis = 0;
long interval = 10000;
 

void setup() {

  pinMode(SIM800L_POWER, OUTPUT);
  digitalWrite(SIM800L_POWER, HIGH);
  
  Serial.begin(9600);
  Serial.println("esp32 serial initialize");
  
  sim800.begin(9600, SERIAL_8N1, SIM800L_RX, SIM800L_TX);
  Serial.println("SIM800L serial initialize");

  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("neogps serial initialize");
  delay(3000);
  
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  
  http_client.setHttpResponseTimeout(90 * 1000); 
}

void loop() {

  Serial.println("Initializing modem...");
  modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" OK");
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" OK");

  
  http_client.connect(FIREBASE_HOST, SSL_PORT);
  

  while (true) {
    if (!http_client.connected()) {
      Serial.println();
      http_client.stop();// Shutdown
      Serial.println("HTTP  not connect");
      break;
    }
    else{
      gps_loop();
    }
  }
}

void PostToFirebase(const char* method, const String & path , const String & data, HttpClient* http) {
  String response;
  int statusCode = 0;
  http->connectionKeepAlive(); // Currently, this is needed for HTTPS
  

  String url;
  if (path[0] != '/') {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=" + FIREBASE_AUTH;
  Serial.print("POST:");
  Serial.println(url);
  Serial.print("Data:");
  Serial.println(data);
  
  String contentType = "application/json";
  http->put(url, contentType, data);
  
  statusCode = http->responseStatusCode();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  response = http->responseBody();
  Serial.print("Response: ");
  Serial.println(response);

  if (!http->connected()) {
    Serial.println();
    http->stop();// Shutdown
    Serial.println("HTTP POST disconnected");
  }
}

void gps_loop()
{

  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }
  }

  if(true){
  newData = false;
  
  String latitude, longitude;

  
  latitude = String(gps.location.lat(), 6); 
  longitude = String(gps.location.lng(), 6); 


  Serial.print("Latitude= "); 
  Serial.print(latitude);
  Serial.print(" Longitude= "); 
  Serial.println(longitude);
      
  String gpsData = "{";
  gpsData += "\"lat\":" + latitude + ",";
  gpsData += "\"lng\":" + longitude + "";
  gpsData += "}";



  //PUT   Write or replace data to a defined path, like messages/users/user1/<data>
  //PATCH   Update some of the keys for a defined path without replacing all of the data.
  //POST  Add to a list of data in our Firebase database. Every time we send a POST request, the Firebase client generates a unique key, like messages/users/<unique-id>/<data>
  
  PostToFirebase("PATCH", FIREBASE_PATH, gpsData, &http_client);
  

  }

}
