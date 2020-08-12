#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const char* ssid = "putYourSSIDHere";
const char* haslo = "putYourPasswordHere";

int minimalHumidityTreshold = 60;

#define button1 14
#define button2 16
#define analogIn A0
#define waterSensor 0
#define humiditySensor 2
#define pumping 12
#define sucking 13
#define motor 15

LiquidCrystal_I2C lcd(0x27,16,2);


const long timezone = 3600 * 2;
int dst = 0;

int wateringDuration = 5 * 1000;
int hour1 = 6;
int hour2 = 21;

char dayOfTheWeek[7][15] = {"Niedziela", "Poniedzialek", "Wtorek", "Sroda", "Czwartek", "Piatek", "Sobota"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", timezone);
ESP8266WebServer server(80);

int hour = 0;
int minute = 0;
int second = 0;
  
int sensorID = 0;
int waterLevel = 0;
int soilHumidity = 0;
int wasPlantWatered = 0;
int button1Pressed = 0;
int button2Pressed = 0;
int button2Released = 0;
unsigned long timeStartingLoop = 0;
unsigned long timeWatering = 0;


void setup() {
  pinMode(button2, INPUT);
  pinMode(analogIn, INPUT);
  pinMode(button1, INPUT);
  pinMode(waterSensor, OUTPUT);
  pinMode(humiditySensor, OUTPUT);
  pinMode(pumping, OUTPUT);
  pinMode(sucking, OUTPUT);
  pinMode(motor, OUTPUT);
  
  Serial.begin(115200);
  Wire.begin(4, 5);
  lcd.begin(16, 2);
  lcd.backlight();
  
  WiFi.begin(ssid, haslo);
  lcd.print("Laczenie z WiFi");
  lcd.setCursor(0, 1);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    lcd.print("*");
    i++;
    if (i > 16){
      lcd.clear();
      lcd.print("Brak WiFi");
      lcd.setCursor(0, 1);
      lcd.print("Sprawdz siec");
    }
  }
  lcd.clear();
  lcd.print("WiFi Polaczone");
  timeClient.begin();
  delay(500);
  lcd.clear();
  
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  
  Serial.println("Twoje IP: ");  Serial.println(WiFi.localIP());
  lcd.print("Twoje IP: ");  
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(5000);
  lcd.clear();
  //kontrolaCzujnikow();
}

void kontrolaCzujnikow(){
  if((millis() - timeStartingLoop >= 50) && (sensorID == 0)){
    digitalWrite(waterSensor, LOW);
    digitalWrite(humiditySensor, HIGH);
    if (millis() - timeStartingLoop >= 1000){
      lcd.setCursor(0, 0); lcd.print("Poziom Wody:     ");
      int wartoscCzujnika = analogRead(analogIn);
      waterLevel = map(wartoscCzujnika, 200, 500, 0, 100);
      if (waterLevel < 0) {waterLevel = 0;}
      if (waterLevel > 100) {waterLevel = 100;}
      lcd.setCursor(12, 0); lcd.print(waterLevel);
      lcd.print("%");
      timeStartingLoop = millis(); sensorID = 1;
      second ++;
    }
  }
  if((millis() - timeStartingLoop >= 50) && (sensorID == 1)){
    digitalWrite(waterSensor, HIGH);
    digitalWrite(humiditySensor, LOW);
    if (millis() - timeStartingLoop >= 1000){
      lcd.setCursor(0, 0); lcd.print("Wilgotnosc:     ");
      int wartoscCzujnika = analogRead(analogIn);
      soilHumidity = map(wartoscCzujnika, 0, 1023, 100, 0);
      lcd.setCursor(12, 0); lcd.print(soilHumidity);
      lcd.print("%");
      timeStartingLoop = millis(); sensorID = 0;
      second ++;
    }
  }
}

void podlewanie(){
  if (timeWatering == 0){
    digitalWrite(pumping, HIGH);
    digitalWrite(sucking, LOW);
    digitalWrite(motor, HIGH);
    timeWatering = millis();
  }
  if(millis() - timeWatering >= wateringDuration){
    digitalWrite(pumping, LOW);
    digitalWrite(sucking, LOW);
    digitalWrite(motor, LOW);
    timeWatering = 0;
    wasPlantWatered = 1;
    button1Pressed = 0;
  }
}

void ustawianieCzasu(){
  timeClient.update();
  hour = timeClient.getHours();
  minute = timeClient.getMinutes();
  second = timeClient.getSeconds(); 
}

void loop() { 
  if (button2Released == 1){
    lcd.clear();
    lcd.noBacklight();
    digitalWrite(pumping, LOW);
    digitalWrite(sucking, LOW);
    digitalWrite(motor, LOW);
    digitalWrite(waterSensor, HIGH);
    digitalWrite(humiditySensor, HIGH);
    if ((digitalRead(button2) == 0) || (button2Pressed == 1)){
      button2Pressed = 1;
      if (digitalRead(button2) == 1){button2Released = 0; button2Pressed = 0;}
    }
  }
  if (button2Released == 0){
    lcd.backlight();
    while(WiFi.status() != WL_CONNECTED){
      lcd.setCursor(0, 0);
      lcd.print("Ponowne laczenie");
      delay(1000);
      lcd.clear();
      delay(1000);
    }
    for(int i = 0; i <= 20000; i++){
      server.handleClient();
    }
    kontrolaCzujnikow();
    ustawianieCzasu();
    lcd.setCursor(0, 1);
    if (waterLevel > 0){
      lcd.print(hour); lcd.print(":");
      lcd.print(minute); lcd.print(":");
      lcd.print(second); lcd.print("            ");
      if ((digitalRead(button1) == 0) || (button1Pressed == 1)){
        button1Pressed = 1;
        if(digitalRead(button1) == 1){
          podlewanie();
        }
      }
      if ((hour == hour1) || (hour == hour2)){
        if ((wasPlantWatered == 0) && (soilHumidity <= minimalHumidityTreshold)){
          podlewanie();
        }
      }
    }
    if (waterLevel == 0){
      lcd.print("Uzupelnij wode! ");
      digitalWrite(pumping, LOW);
      digitalWrite(sucking, LOW);
      digitalWrite(motor, LOW);
    }
    if ((hour != hour1) && (hour != hour2)){
        wasPlantWatered = 0;
    }
    if ((digitalRead(button2) == 0) || (button2Pressed == 1)){
      button2Pressed = 1;
    if (digitalRead(button2) == 1){button2Released = 1; button2Pressed = 0;}
    }
  }
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
  Serial.println("Server OK");
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
  Serial.println("Server NOT OK");
}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta http-equiv=\"refresh\" content=\"2\", name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Monitoring doniczki</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Monitoring doniczki</h1>\n";

  if(waterLevel > 0){
    ptr +="<p>Poziom Wody: ";
    ptr +=(int)waterLevel;
    ptr +="%</p>";
  }
  else{
    ptr +="<p>Poziom Wody: ";
    ptr +="BRAK, Uzupelnij wode";
  }
  ptr +="<p>Wilgotnosc Gleby: ";
  ptr +=(int)soilHumidity;
  ptr +="%</p>";
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
