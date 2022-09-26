#include <Wire.h>
#include <MAX30100_PulseOximeter.h>

#define REPORTING_PERIOD_MS   3000
PulseOximeter pox;
uint32_t tsLastReport = 0;

#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include<LiquidCrystal.h>
LiquidCrystal lcd(2,3,4,5,6,7);

unsigned long fix_age;

SoftwareSerial GSM(8,9);

TinyGPS gps;
void gpsdump(TinyGPS &gps);
bool feedgps();
void getGPS();
long lat, lon;
float LAT, LON;

char inchar; // Will hold the incoming character from the GSM shield

int bpm, spO2; 

int buz = 13;
int V_S = A0;
int button = A1;
int F_S = A2;

int S1=0, S2=0, timer0=0, timer1=10, start=0, Send=0, ms=0;

char *phone_no[] = {
  "03474714970", 
  "03000931172", 
  "03107675975", 
  };

char *msg_no[] = {
  "Reply", 
  "Emergency...", 
  "Vehicle Accident Alert"
  }; 

void onBeatDetected(){Serial.println("Beat!");}

void setup(){   
pinMode(V_S, INPUT_PULLUP);
pinMode(F_S, INPUT_PULLUP);
pinMode(button, INPUT_PULLUP);
pinMode(buz, OUTPUT);

  lcd.begin(16,2);  
  lcd.print("Accident Alert  ");
  lcd.setCursor(0,1);
  lcd.print("     System     ");
  delay(2000);
  lcd.clear();
  lcd.print("GSM Initializing");
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");
  delay(1000);
 
 GSM.begin(9600);
 Serial.begin(9600);
 Serial.println("Initializing....");
 initModule("AT","OK",1000);
 initModule("ATE1","OK",1000);
 initModule("AT+CPIN?","READY",1000);  
 initModule("AT+CMGF=1","OK",1000);     
 initModule("AT+CNMI=2,2,0,0,0","OK",1000);  
 Serial.println("Initialized Successfully");
   
 GSM.print("AT+CMGS=\"");GSM.print(phone_no[0]);GSM.println("\"\r\n");
 delay(1000);
 GSM.println("Welcome to  Vehicle Accident Alert System using GPS, GSM and Shock Sensor");
 delay(300);
 GSM.write(byte(26));
 delay(3000); 
 getGPS(); 

Serial.print("Initializing pulse oximeter..");  

lcd.clear();
lcd.print("Initialized");
lcd.setCursor(0,1);
lcd.print("Successfully");
delay(2000);
lcd.clear();

if (!pox.begin()) {
Serial.println("FAILED");
for(;;);
}else{Serial.println("SUCCESS");}
pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop(){

if(digitalRead(button) == 0){digitalWrite(buz, LOW); ms=0, S1=0, S2=0; timer1=10; timer0=0; start=0;delay(1000);} 

if(digitalRead(V_S) == 0){S1=1;}

if(digitalRead(F_S) == 1){
if(ms<500)ms=ms+1;
if(ms==500)S2=1;  
}else{ms=0;}

if(S2==1){digitalWrite(buz, HIGH);}

if(bpm>30 && bpm<55){digitalWrite(buz, HIGH);}

lcd.setCursor(0,0);
lcd.print("Shock:");
lcd.print(S1);

lcd.setCursor(0,1);
lcd.print("BPM:");
lcd.print(bpm);
lcd.print("  ");

lcd.setCursor(8,1);
lcd.print("spO2:");
lcd.print(spO2);
lcd.print("%  ");

lcd.setCursor(10,0);
if(start==0){  
if(S1==1){digitalWrite(buz, HIGH);
if(timer0>100){timer0=0; timer1=timer1-1;}
if(timer1==0){timer1=0;
getGPS();  
Send=2; sms(); start=1; lcd.clear();} 

timer0 = timer0+1;
lcd.print("T:");lcd.print(timer1);lcd.print("  ");
}
}else{lcd.print("       ");}


 if(GSM.available() >0){inchar=GSM.read();
 if(inchar=='R'){inchar=GSM.read(); 
 if(inchar=='I'){inchar=GSM.read();
 if(inchar=='N'){inchar=GSM.read();
 if(inchar=='G'){  
 GSM.print("ATH\r");
 delay(1000);
 Send=0; sms();
   }
  }
 }
 }
 
}


      // Make sure to call update as fast as possible
pox.update();
if(millis() - tsLastReport > REPORTING_PERIOD_MS) {   
bpm = pox.getHeartRate();
spO2= pox.getSpO2(); 

Serial.print(bpm);   Serial.print(" \t ");
Serial.println(spO2);
tsLastReport = millis(); 
}

}


void sms(){
lcd.clear();
lcd.print("Getting GPS Data");
lcd.setCursor(0,1);
lcd.print("Please Wait.....");

getGPS();     
lcd.clear();
lcd.print("Sending SMS ");
GSM.print("AT+CMGS=\"");GSM.print(phone_no[0]);GSM.println("\"\r\n");
delay(1000);
GSM.println(msg_no[Send]);
GSM.print("http://maps.google.com/?q=loc:");
GSM.print(LAT/1000000,7);
GSM.print(",");
GSM.println(LON/1000000,7);
delay(300);
GSM.write(byte(26));
delay(5000);  

GSM.print("AT+CMGS=\"");GSM.print(phone_no[1]);GSM.println("\"\r\n");
delay(1000);
GSM.println(msg_no[Send]);
GSM.print("http://maps.google.com/?q=loc:");
GSM.print(LAT/1000000,7);
GSM.print(",");
GSM.println(LON/1000000,7);
delay(300);
GSM.write(byte(26));
delay(5000);  

GSM.print("AT+CMGS=\"");GSM.print(phone_no[2]);GSM.println("\"\r\n");
delay(1000);
GSM.println(msg_no[Send]);
GSM.print("http://maps.google.com/?q=loc:");
GSM.print(LAT/1000000,7);
GSM.print(",");
GSM.println(LON/1000000,7);
delay(300);
GSM.write(byte(26));
delay(5000);  
}


void getGPS(){
bool newdata = false;
unsigned long start = millis();
// Every 1 seconds we print an update
while (millis() - start < 1000){
    if (feedgps ()){
      newdata = true;
    }
  }
if (newdata){
    gpsdump(gps);
  }
}

bool feedgps(){
while (Serial.available()){
    if (gps.encode(Serial.read()))
      return true;
  }
  return 0;
}

void gpsdump(TinyGPS &gps){
  //byte month, day, hour, minute, second, hundredths;
  gps.get_position(&lat, &lon);
  LAT = lat;
  LON = lon;
  {
    feedgps(); // If we don't feed the gps during this long routine, we may drop characters and get checksum errors
  }
}

void initModule(String cmd, char *res, int t){
//while(1){
    Serial.println(cmd);
    GSM.println(cmd);
    delay(100);
    while(GSM.available()>0){
       if(GSM.find(res)){
        Serial.println(res);
        delay(t);
        return;
       }else{
        Serial.println("Error");
        }}
    delay(t);
 // }
}
