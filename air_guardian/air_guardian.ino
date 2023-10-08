//add to File-Preferences..-Additional Manager Boards URL:
//https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json

//add library from zip dowloaded on: https://github.com/Seeed-Studio/SGP30_Gas_Sensor
#include <i2c.h>
#include <sensirion_common.h>
#include <sensirion_configuration.h>
#include <sgp30.h>
#include <sgp_featureset.h>

#include <wiring_pwm.h>

/*

    This sketch is for air guardian using Seeed Studio WIO
    
    #######################################
    ###### IoT Into the Wild Contest ######
    #######################################

    Gabriel Coello

*/


#include "SPI.h"
#include "TFT_eSPI.h"
#include"Free_Fonts.h" 
#define TFT_GREY 0x7BEF
TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library

unsigned long runTime = 0;

int data_eCO2[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int index_min_eCO2 = 0;
int index_max_eCO2 = 0;
int data_TVOC[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int index_min_TVOC = 0;
int index_max_TVOC = 0;

int n_skip_ejecution = 0;

int g_activated = 0;

void setup() {

    Serial.begin(115200);
    delay(500);
    Serial.println("serial start!!");

    //randomSeed(analogRead(A0));
    // Setup the LCD
    myGLCD.init();
    myGLCD.setRotation(3);

    s16 err;
    u16 scaled_ethanol_signal, scaled_h2_signal;

    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != STATUS_OK) {
        Serial.println("SGP failed");
        while (1);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);
    if (err == STATUS_OK) {
        Serial.println("get ram signal!");
    } else {
        Serial.println("error reading signals");
    }
    err = sgp_iaq_init();
    
    //WIO Terminal 5 Way button init
    pinMode(WIO_5S_UP, INPUT_PULLUP);
    pinMode(WIO_5S_DOWN, INPUT_PULLUP);
    pinMode(WIO_5S_LEFT, INPUT_PULLUP);
    pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
    pinMode(WIO_5S_PRESS, INPUT_PULLUP);
    //Pins for motor driver L298N
    pinMode(BCM13, OUTPUT); //PIN33 to L298N:IN4   MOTOR2 A1
    pinMode(BCM16, OUTPUT); //PIN36 to L298N:IN3   MOTOR2 A2
    //BCM26 PIN37 to L298N:EN2   MOTOR2 ENABLE for PWM speed control
    pinMode(BCM20, OUTPUT); //PIN35 to L298N:IN2   MOTOR1 B1
    pwm(BCM26, 100);  // attaches the servo on pin 9 to the servo object

}

void loop() {
    char buf[16];
    s16 err = 0;
    u16 tvoc_ppb, co2_eq_ppm;
    if(n_skip_ejecution == 10){//Parameters are checked each 5 seconds
      n_skip_ejecution = 0;
      err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
      if (err == STATUS_OK) {
          Serial.print("tVOC:");
          Serial.print(tvoc_ppb);
          Serial.print("ppb  -  ");
  
          Serial.print("CO2eq:");
          Serial.print(co2_eq_ppm);
          Serial.println("ppm");

          if(tvoc_ppb > 500){ //Id TVOC is too high spread water later
            g_activated = 1;
          }
      } else {
          Serial.println("error reading IAQ values\n");
      }
  
      
      if(g_activated == 1){
        myGLCD.fillRoundRect(1, 1, 320, 34, 3, TFT_RED);
        myGLCD.fillRoundRect(1, 116, 320, 10, 3, TFT_RED);
        myGLCD.fillRoundRect(1, 206, 320, 34, 3, TFT_RED);
      }else{
        myGLCD.fillRoundRect(1, 1, 320, 34, 3, TFT_GREEN);
        myGLCD.fillRoundRect(1, 116, 320, 10, 3, TFT_GREEN);
        myGLCD.fillRoundRect(1, 206, 320, 34, 3, TFT_GREEN);
      }
        
      myGLCD.fillRoundRect(1, 35, 320, 80, 3, TFT_BLUE);
      myGLCD.fillRoundRect(1, 125, 320, 80, 3, TFT_BLUE);
      myGLCD.setFreeFont(FS12);
      myGLCD.setTextSize(6);
      myGLCD.setTextColor(TFT_WHITE, TFT_WHITE);
      itoa(tvoc_ppb, buf, 10);
      myGLCD.drawCentreString(buf, 160, 30, 2);
      myGLCD.setTextColor(TFT_WHITE, TFT_WHITE);
      itoa(co2_eq_ppm, buf, 10);
      myGLCD.drawCentreString(buf, 160, 119, 2);
  }else{
    n_skip_ejecution = n_skip_ejecution + 1;
  }
  
  //Check 5 way button or if TVCO is too high
  if (digitalRead(WIO_5S_DOWN) == LOW or g_activated == 1) {
    g_activated = 0;
    //Motor control for turn on the fan and then motor for spread the could of water
    digitalWrite(BCM13, 0);
    digitalWrite(BCM16, 0);
    digitalWrite(BCM20, 1);
    Serial.println("5 Way Down");
    delay(500);
    //activate motor for spread
    digitalWrite(BCM13, 1);
    digitalWrite(BCM16, 0);
    delay(350);
    digitalWrite(BCM13, 0);
    digitalWrite(BCM16, 0);
    delay(200);
    //return de motor to original position
    digitalWrite(BCM13, 0);
    digitalWrite(BCM16, 1);
    delay(250);
    digitalWrite(BCM13, 0);
    digitalWrite(BCM16, 0);
    delay(200);
    digitalWrite(BCM20, 0);
    //myGLCD.fillScreen(TFT_WHITE);        
   }

  delay(200);
}

int getAverage(int data[], int max_indice){
    int tmp_average = 0;
    for(int i = 0; i < max_indice; i++){
         tmp_average = tmp_average + data[i];
    }
    double d_average = tmp_average / (max_indice + 1);
    return round(d_average);
}
