/*
 Test and validation of the SI4735 Arduino Library with SSB support.
 SSB support has been successfully tested on SI4735-D60 and Si4732-A10 devices.

 
 Features of this sketch:

1) Only SSB (LSB and USB);
2) Audio bandwidth filter 0.5, 1, 1.2, 2.2, 3 and 4kHz;
3) Eight ham radio bands pre configured;
4) BFO Control; and
5) Frequency step switch (1, 5 and 10kHz);
 
 */

#include <Arduino.h>
#include <U8g2lib.h>

#include <SPI.h>
#include <Wire.h> //for i2c scan

#include <SI4735.h>
#include <patch_full.h> 

const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h



U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 53, /* dc=*/ 46, /* reset=*/ 48);  

/*BITMAPS for OLED display.
 * Use GIMP to generate them. The format is : X Bitmap
*/
#define Speaker_Stby_width 22
#define Speaker_Stby_height 8
static const unsigned char Speaker_Stby[] U8X8_PROGMEM  = {
 0x40, 0x00, 0x0e, 0x60, 0x21, 0x1f, 0x70, 0x12, 0x11, 0x7a, 0x0c, 0x11,
 0x7a, 0x0c, 0x11, 0x70, 0x12, 0x11, 0x60, 0x21, 0x15, 0x40, 0x00, 0x1f};


#define Speaker_Active_width 14
#define Speaker_Active_height 8
static const unsigned char Speaker_Active[] U8X8_PROGMEM = {
   0x20, 0x08, 0x30, 0x10, 0x3b, 0x22, 0xbd, 0x24, 0xbd, 0x24, 0x3b, 0x22,
   0x30, 0x10, 0x20, 0x08 };

#define Speaker_Off_width 10
#define Speaker_Off_height 10
static const unsigned char Speaker_Off[] U8X8_PROGMEM = {
   0x01, 0x02, 0x42, 0x01, 0x64, 0x00, 0x50, 0x00, 0x56, 0x00, 0x5e, 0x00,
   0x56, 0x00, 0x50, 0x00, 0x62, 0x01, 0x41, 0x02}; 

#define Volume_width 10
#define Volume_height 8
static const unsigned char Volume[] U8X8_PROGMEM  = {
 0x78, 0x00, 0x84, 0x00, 0x02, 0x01, 0x02, 0x01, 0x87, 0x03, 0x87, 0x03,
 0x87, 0x03, 0x87, 0x03};


#define LSB_width 11
#define LSB_height 8
static const unsigned char LSB[] U8X8_PROGMEM  = {
0x00, 0x00, 0x28, 0x00, 0x08, 0x00, 0x2c, 0x00, 0x0c, 0x00, 0x2e, 0x00,
0x0e, 0x00, 0x2f, 0x00};

#define USB_width 11
#define USB_height 8
static const unsigned char USB[] U8X8_PROGMEM  = {
0x00, 0x00, 0xa0, 0x00, 0x80, 0x00, 0xa0, 0x01, 0x80, 0x01, 0xa0, 0x03,
0x80, 0x03, 0xa0, 0x07};


#define AM_width 11
#define AM_height 8
static const unsigned char AM[] U8X8_PROGMEM  = {
0x20, 0x00, 0xa8, 0x00, 0xa8, 0x00, 0xac, 0x01, 0xac, 0x01, 0xae, 0x03,
0xae, 0x03, 0xaf, 0x07};







int LowVoltageIcon = 0;

float SystemVoltage (void) {
analogReference(INTERNAL2V56);
//analogReference(INTERNAL1V1);
int AnalogPin=A0;
int val=analogRead(AnalogPin);
float Voltage=val*10*2.56/1024;
return(Voltage);  
}

void speaker_indicator (void){
int Mute = 0;
if (Mute == 1){
    u8g2.drawXBMP(70,1, Speaker_Off_width, Speaker_Off_height, Speaker_Off);
} else {
    u8g2.drawXBMP(70,1, Speaker_Active_width, Speaker_Active_height, Speaker_Active);
}

}

void speaker_st_blink(void){
LowVoltageIcon=LowVoltageIcon^1;
/* Print bitmap Speaker disabled */
u8g2.setColorIndex(LowVoltageIcon);
//u8g2.setDrawColor(0);
u8g2.drawXBMP(70,1, Speaker_Stby_width, Speaker_Stby_height, Speaker_Stby);  
}



void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void setup(void) {
  
  u8g2.begin(); //Activate the Display
  
  Wire.begin(); //Activate the SPI bus
  
  Serial.begin(9600); // Activate Console
  
}


uint8_t ColdStart=0;
uint8_t Init=0;

void WelcomeScreen(void){
  if (ColdStart==0){
 u8g2.setFont(u8g2_font_6x10_tf); 
 u8g2.drawStr( 10, 0, "Welcome to Si4735");
 u8g2.drawStr( 45, 8, "radio!");
 u8g2.drawStr( 30, 45, "Designed by ");
 u8g2.drawStr( 25, 54, "Andrey Ivanov.");
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
 
 ColdStart=1;  // Hide start dialog from loop
 }  
}

void i2c_check(void){
 
 if (Init==0){
 //u8g2.setFont(u8g2_font_5x7_mf); 
 u8g2.setCursor(0,0);
 u8g2.print( "Initialisation...");
 u8g2.sendBuffer();
 delay(200);
 u8g2.setCursor(0,9);
 u8g2.print( "Starting looking for");
 u8g2.setCursor(0,19);
 u8g2.print( "i2c devices.");
 u8g2.sendBuffer();
 delay(500);
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
 
}


float Voltage=SystemVoltage();
u8g2.setCursor(0,54);
u8g2.print    ("Voltage:");
u8g2.setCursor(60,54);
u8g2.print    (Voltage);
u8g2.sendBuffer();
//
Init=1; //Hide i2c search dialog from loop
delay(1000);
}


}


void battery_indicator (void){

float Voltage=SystemVoltage(); /*Check system voltage!*/
if (Voltage < 11.0){
speaker_st_blink();
}

if (Voltage > 11.0){

speaker_indicator();

}

u8g2.setCursor(98,1);
u8g2.print((String)Voltage+"V");
u8g2.setDrawColor(1);
}










 
void testdisplay(void){
long et1=0;
long et2=0;
et1 = millis();  
u8g2.clearBuffer();
u8g2.setFont(u8g2_font_5x7_mf);
  
u8g2.setCursor(0,0);
u8g2.drawLine(0, 0, 128, 0);
u8g2.drawLine(0, 10, 128, 10);
u8g2.setCursor(0,1);
u8g2.print("SSB"); // Print Mode AM/FM/MW/LSB/USB

//u8g2.drawXBMP(17,1, LSB_width, LSB_height, LSB);
//u8g2.drawXBMP(17,1, USB_width, USB_height, USB);
u8g2.drawXBMP(17,1, AM_width, AM_height, AM);

u8g2.drawXBMP(32,1, Volume_width,Volume_height, Volume); /*Print Bitmap "Headphones"*/
u8g2.setCursor(42,2); 
u8g2.print("100%");

battery_indicator();
//speaker_indicator();

/*    SSB: SPEAKER VOLUME Voltage
 *    SNR:20 RSSI: 20 dBuV  AGC: ON/OFF                     
 *    LNA 80 BW 6 kHz  BFO +/- 6 kHz
 *                                    
 */
//u8g2.drawXBMP(50,1, Volume_width,Volume_height, Volume); /*Print Bitmap "Headphones"*/

/*Print frequency*/
u8g2.setCursor(0,32); 
u8g2.setFont(u8g2_font_courB10_tf);
u8g2.print("80.1 MHz");
u8g2.setFont(u8g2_font_5x7_mf);

/*Print LNA*/
u8g2.setCursor(0,10);
u8g2.print("LNA:");
u8g2.setCursor(25,10);
u8g2.print("99"); // Print AGC Level
//u8g2.print    ("Voltage:");
u8g2.setCursor(0,18);
u8g2.print("BW:");
u8g2.setCursor(25,18);
u8g2.print("6kHz"); // Print BW
//u8g2.print    ("Voltage:");  

u8g2.setCursor(85,10);
u8g2.print("AGC:");
u8g2.setCursor(108,10);
u8g2.print("OFF"); // Print ON/OFF
et2 = millis();
Serial.print( (et2 - et1) );
Serial.println("ms");
}





void draw(void) {
 u8g2_prepare();
 
 if (ColdStart==0) {
 WelcomeScreen();
 };

 if(Init==0){
 /// Search I2c devices...
 i2c_check();

 };


if(Init==1){
 /// Search I2c devices...
 testdisplay();

 };
 
 






}


void loop(void) {
 

  // picture loop  
  u8g2.clearBuffer();
  draw();
  u8g2.sendBuffer();

  delay(1000);

}
