

#include <Arduino.h>
#include <U8g2lib.h>

#include <SPI.h>
#include <Wire.h> //for i2c scan

U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 53, /* dc=*/ 46, /* reset=*/ 48);  



float SystemVoltage (void) {

analogReference(INTERNAL2V56);
//analogReference(INTERNAL1V1);
int AnalogPin=A0;
int val=analogRead(AnalogPin);
float Voltage=val*10*2.56/1024;
return(Voltage);  
}



void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup(void) {
  u8g2.begin();
  Wire.begin(); 
  Serial.begin(9600);
  
}


uint8_t ColdStart=0;
uint8_t Init=0;

void WelcomeScreen(void){
if (ColdStart==0){
 u8g2.drawStr( 10, 0, "Welcome to Si4735");
 u8g2.drawStr( 45, 8, "radio!");
 u8g2.drawStr( 30, 45, "Designed by ");
 u8g2.drawStr( 25, 54, "Andrey Ivanov.");
 u8g2.setCursor(0,0);
 u8g2.drawStr( 25, 30, "Start in");
 u8g2.drawStr( 86, 30, "s.");
 for (int timer=5; timer>0; timer--){  /* Update timer variable and appears value.*/
 u8g2.setCursor(80,30);
 u8g2.print (timer);
 delay(1000);
 u8g2.sendBuffer();
 }; 
 delay(100);
 u8g2.clearBuffer();  
 ColdStart=1;  
 }  
}










void i2cinit(void){

 if (Init==0){
 u8g2.setCursor(0,0);
 u8g2.print( "Initialisation...");
 u8g2.sendBuffer();
 delay(200);
 u8g2.setCursor(0,9);
 u8g2.print( "Starting looking for");
 u8g2.setCursor(0,19);
 u8g2.print( "i2c devices.");
 delay(200);
 u8g2.sendBuffer();
 delay(200);
 u8g2.clearBuffer();
 byte error, address;
 int nDevices;
 nDevices = 0; //must be 0

 for(address = 16; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {   
      
        u8g2.setCursor(0,0);
        u8g2.print    ("i2c device found at");
        u8g2.setCursor(0,8);
        u8g2.print    ("address:");
        u8g2.setCursor(0,20+10*nDevices);
        u8g2.print    ("0x");
        u8g2.setCursor(12,20+10*nDevices);
        u8g2.print    (address,HEX);
        if(address==0x63){
        u8g2.setCursor(60,20+10*nDevices);
        u8g2.print    ("[SI4735]");         
        }
        if(address==0x50){
        u8g2.setCursor(60,20+10*nDevices);
        u8g2.print    ("[EPROM]");         
        }
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
  {
    u8g2.setCursor(0,50);
    u8g2.print    ("No i2c devices found!");
    
  } 
    
  else {
  
    u8g2.setCursor(0,20+10*nDevices);
    u8g2.print    ("done."); 
    delay(500);
}


float Voltage=SystemVoltage();
u8g2.setCursor(0,54);
u8g2.print    ("Voltage:");
u8g2.setCursor(60,54);
u8g2.print    (Voltage);
delay(200);
Init=0; //Should be 1
}
}
 



void draw(void) {
 u8g2_prepare();
 
 if (ColdStart==0) {
 WelcomeScreen();
 };

 

 if(Init==0){
 /// Search I2c devices...
 i2cinit();
  
 };









}


void loop(void) {
 

  // picture loop  
  u8g2.clearBuffer();
  draw();
  u8g2.sendBuffer();
 
  delay(1000);

}
