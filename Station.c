#include <SPI.h>
#include <WiFiNINA.h>                 // #include <utility/wifi_drv.h> ha csak a builtin led kell...
#include <PubSubClient.h>             // MQTT https://pubsubclient.knolleary.net/api
#include <Arduino_MKRIoTCarrier.h>
 
MKRIoTCarrier carrier;
const char ssid[] = "labor2";                       
const char pass[] = "12345678";          
const char* MQTT_broker     = "FS1.csany-zeg.local";   // "10.2.0.1"; 
const char* MQTT_client_ID  = "meto";  // Egyedi legyen !
char* MQTT_topic_SSR  = "meto/SSR";    // subscribe
char* MQTT_topic_full = "meto/full";   // subscribe
const int CIKLUSIDO   = 5000;    
const int OUTPUT_PIN = 1;         // 5 mp
unsigned long startms = millis();   
float t=0, h=1, p=2;
float t2=0, h2=1, p2=2;
int counter = 0;

int prevTouch = 0;

bool powerSave = true;
bool nightMode = false;


uint32_t Red    = carrier.leds.Color(0, 255, 0);  // RGB PWM vezérelt ledek…
uint32_t Green  = carrier.leds.Color(255, 0, 0);  
uint32_t Blue   = carrier.leds.Color(0, 0, 255);
uint32_t Yellow = carrier.leds.Color(255, 255, 0);
uint32_t White  = carrier.leds.Color(155, 100, 50);
uint32_t Black = carrier.leds.Color(0, 0, 0);
uint32_t OffCol = carrier.leds.Color(0, 0, 0);    //OFF
 
void led_on(byte i, uint32_t szin) 
{
  carrier.leds.fill(OffCol, 0, 5);
  carrier.leds.setPixelColor(i, szin);
  carrier.leds.show();
}

  
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
 
void builtinLED(byte i, byte maxfeny) // 0:R,1:G,2:B ; maxfény:0..255
{
  WiFiDrv::analogWrite(25, 0);
  WiFiDrv::analogWrite(26, 0);
  WiFiDrv::analogWrite(27, 0);
  if (i >=0 && i <= 2) { WiFiDrv::analogWrite(25+i, maxfeny); }
}
 
void printWifiStatus() {
  Serial.print("SSID: ");  Serial.println(WiFi.SSID());
  Serial.print("signal strength (RSSI):");Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP()); 
}
 
void mqtt_connect() {                                    
  Serial.println("Connecting to MQTT broker:"); Serial.print(MQTT_broker);
  while (!mqtt_client.connected()) { 
      if (mqtt_client.connect(MQTT_client_ID)) {      //  +, user, passwd!
          mqtt_client.subscribe(MQTT_topic_SSR, 0);   // + qos=0
          mqtt_client.subscribe(MQTT_topic_full, 0);  // + qos=0   
      }
      delay(500); 
      Serial.print("."); 
  }
  Serial.println("Succesfully...");
}
 
void MQTT_callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) { messageTemp += (char)message[i];  }
  if (strcmp(topic, MQTT_topic_SSR) ==0)
  {       
      if ((char) messageTemp[0] == '1')  { updateon();  }
      if ((char) messageTemp[0] == '0')  { updateoff(); }
      if ((char) messageTemp[0] == '4')  { nightMode = false; clear(); carrier.display.println("Night Mode: OFF");delay(1000); clear(); }
      if ((char) messageTemp[0] == '3')  { 
        nightMode = true;
        led_on(0, OffCol);
        led_on(1, OffCol);
        led_on(2, OffCol);
        led_on(3, OffCol);
        led_on(4, OffCol);
        clear();
        carrier.display.println("Night Mode: ON");
        delay(1000);
        clear();
      }
      if ((char) messageTemp[0] == '5'){reset();}
  }
}

  
void setup() {
  WiFiDrv::pinMode(25, OUTPUT); // green pin
  WiFiDrv::pinMode(26, OUTPUT); // red pin
  WiFiDrv::pinMode(27, OUTPUT); // blue pin
  Serial.begin(9600); delay(500);
  Serial.print("Attempting to connect to SSID: ");  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  CARRIER_CASE = true;  // dobozban van?
  carrier.begin();
  carrier.display.fillScreen(ST77XX_BLACK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(1000);
  }
  printWifiStatus();
  builtinLED(2,10);    // wifi ok
  mqtt_client.setServer(MQTT_broker, 8883);
  mqtt_client.setCallback(MQTT_callback);
  mqtt_connect();
  builtinLED(0,10);    // mqtt ok
  digitalWrite(OUTPUT_PIN, HIGH);
  pinMode(OUTPUT_PIN, OUTPUT);
}

void reset(){

  carrier.display.println("Mukodik");
  digitalWrite(OUTPUT_PIN, LOW);
}

void updateon()  { 
    clear();
    carrier.display.println("Update on");
    delay(1000);
    clear();
}  

void updateoff() {
    clear();
    carrier.display.println("Update off");
    delay(1000);
    clear();
}

void clear(){
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setCursor(10, 100);
}

void clearnum(){
  carrier.display.fillRect(80, 150, 100, 50, ST77XX_BLACK);
}

void writeTemp(){
  if (carrier.Buttons.getTouch(TOUCH4) && t2 != t){
    t2 = t;
    if (prevTouch != 0 ){
       
        clear();
        carrier.display.println(" Temperature"); carrier.display.setCursor(80, 150); carrier.display.println(t); 
        if (nightMode == false){
          led_on(4,Yellow);
        }
        
      
      
    }
    else {
      
        clearnum();
        carrier.display.setCursor(10, 100); carrier.display.println(" Temperature"); carrier.display.setCursor(80, 150); carrier.display.println(t);
        if(nightMode == false){
          led_on(4,Yellow);
        } 
        
      }
    
    prevTouch = 0;
  }
}

void writeHumidity(){
    if (carrier.Buttons.getTouch(TOUCH3) && h2 != h){
    h2 = h;
    if (prevTouch != 1){
    
        clear();
        carrier.display.println("  Humidity"); carrier.display.setCursor(80, 150); carrier.display.println(h); 
        if(nightMode == false){
          led_on(3,Red);
        } 
        
      
      
    }
    else {
     
        clearnum();
        carrier.display.setCursor(10, 100); carrier.display.println("  Humidity"); carrier.display.setCursor(80, 150); carrier.display.println(h); 
        if(nightMode == false){
          led_on(3,Red);
        }
        
      
      
    }
    prevTouch = 1;
  }
}


void writePressure(){
    if (carrier.Buttons.getTouch(TOUCH2) && p2 != p){
    p2 = p;
    if (prevTouch != 2){
    
        clear();
        carrier.display.println("  Pressure"); carrier.display.setCursor(80, 150); carrier.display.println(p); 
        if (nightMode == false){
          led_on(2,Blue);
        }
        
      }
      
    
    else {
     
        clearnum();
        carrier.display.setCursor(10, 100); carrier.display.println("  Pressure"); carrier.display.setCursor(80, 150); carrier.display.println(p);
        if (nightMode == false){
          led_on(2,Blue);
        }
      }
      
    
    prevTouch = 2;
  }
}

void writeTempU(){
  if (prevTouch == 0 && powerSave == false && t2 != t){
    t2 = t;
    if (prevTouch != 0){
      clear();
      carrier.display.println(" Temperature"); carrier.display.setCursor(80, 150); carrier.display.println(t);
      if (nightMode == false){
          led_on(4,Yellow);
        }
    }
    else{
      clearnum();
      carrier.display.setCursor(10, 100); carrier.display.println(" Temperature"); carrier.display.setCursor(80, 150); carrier.display.println(t);
      if (nightMode == false){
          led_on(4,Yellow);
        }
    }
    
  }
}

void writeHumU(){
  if (prevTouch == 1 && powerSave == false && h2 != h){
    h2 = h;
    if (prevTouch != 1){
      clear();
      carrier.display.println("  Humidity"); carrier.display.setCursor(80, 150); carrier.display.println(h); 
      if (nightMode == false){
          led_on(3,Red);
        }
    }
    else{
      clearnum();
      carrier.display.setCursor(10, 100); carrier.display.println("  Humidity"); carrier.display.setCursor(80, 150); carrier.display.println(h);
      if (nightMode == false){
          led_on(3,Red);
        }
    }
    
  }
}

void writePresU(){
  if (prevTouch == 2 && powerSave == false && p2 != p){
    p2 = p;
    if (prevTouch != 2){
      clear();
      carrier.display.println("  Pressure"); carrier.display.setCursor(80, 150); carrier.display.println(p); 
      if (nightMode == false){
          led_on(2,Blue);
        }
    }
    else{
      clearnum();
      carrier.display.setCursor(10, 100); carrier.display.println("  Pressure"); carrier.display.setCursor(80, 150); carrier.display.println(p);
      if (nightMode == false){
          led_on(2,Blue);
        }
    }
    
  }
}

void loop() {
  carrier.Buttons.update();
             // Read the buttons state

  
  writeTemp();
  writeHumidity();
  writePressure();

  writeTempU();
  writeHumU();
  writePresU();
  
  
  if (carrier.Buttons.getTouch(TOUCH0) && powerSave != false){
    powerSave = false;
    prevTouch = 3;
    clear();
    carrier.display.println("Power Saving Mode: OFF");
    delay(1000);
    clear();
  }

  if (carrier.Buttons.getTouch(TOUCH1) && powerSave != true){
    powerSave = true;
    prevTouch = 3;
    clear();
    carrier.display.println("Power Saving Mode: ON");
    delay(1000);
    clear();
  }


  if (millis() - startms >= CIKLUSIDO ) 
  {
    carrier.display.setCursor(30, 100);
    carrier.display.setTextSize(3);
    startms = millis();
    t = carrier.Env.readTemperature();    
    h = carrier.Env.readHumidity();
    p = carrier.Pressure.readPressure();
    char sor[128]; 
    sprintf(sor, "{\"ID:\":\"%d\",\"C1\":\"%5.2f\",\"H1\":\"%d\",\"P1\":\"%d\",\"O1\":\"%d\"}\n", ++counter,t, (int)h, (int)p, 0);
    Serial.print(sor);
    mqtt_client.publish(MQTT_topic_full, sor); 
  }
  if (!mqtt_client.connected()) { mqtt_connect();  }
  if(!mqtt_client.loop())       { mqtt_client.connect(MQTT_client_ID); }
}