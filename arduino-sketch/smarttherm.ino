/*
 * ********* Twitter Reader ********
 * by Niek Temme http://niektemme.com
 *
 * smart thermostaat
 * 
 */
#include <XBee.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// create a software serial port for the XBee
SoftwareSerial Serial3(6, 7);
XBee xbee = XBee();
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40B7FB33);
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();
// connect to an LCD using the following pins for rs, enable, d4, d5, d6, d7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// defines the character width of the LCD display
#define WIDTH 16
#define aref_voltage 3.3       
// resistance at 25 degrees C
//#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
//#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
//#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
//#define BCOEFFICIENT 3950
// the value of the 'other' resistor
//#define SERIESRESISTOR 10000 

int sensorPin = 0; //sensor = settemp
int sensorValue = 0;
int sensorOldValue = 0;
int temperaturePin = 1; //act temp
int tempValue = 0;
int tempOldValue = 0;
int tempDisValue = 0;
int tempDisOldValue = 0;
int boilerPin = 2; //sensor = settemp
int boilerValue = 0;
int thermistorPin= 3;
int thermistorValue =0;
int thermistor2Pin= 4;
int thermistor2Value =0;
//char sendTempValue[10];
//char sendSensorValue[10];
//char sendBoilerValue[10];
char sendValue[10];
char textset[24];
char texttemp[20];
char temstr[4];
char tmvstr[4];

int avgTotalTemp = 0;
int avgTemp = 0;
int avgNumSamp = 0;
int avgMaxSamp =10;


int sendp = 0;

long previousMillisDis = 0;
long previousMillisTemp = 0;
long intervaldis = 100;
long intervaltemp = 2000;
long previousMillisSend = 0;
long intervalSend = 100;

long previousMillisBlink = 0;
long intervalBlink = 10000;

int startup = 0;
int controllPin = 9;
int runScen = 0;
unsigned long startScen = 0;
unsigned long scenLength = 0;
unsigned long maxScen = 600000;
int boilerStat = 0;
//int overshoot = 0;

int mesAvailable = 0;
char chour[3];
char cminute[3];
char cboiler[2];
char crtemp[5];
char cvchecksum[5];
int vhour = 0;
int vminute =0;
int vminuteold =0;
int rvboiler = 0;
int rvtemp = 0;
int rvchecksum=0;

int remotemode = 0;
int remotemodeold = 0;
unsigned int rcycle = 3000;
unsigned int maxrcycle = 2000;

int vrunl = 0;
unsigned long startArdMillis = 0;
unsigned long nextRunl = 10000;

char lpayload[60]; 

void setup() {
   analogReference(EXTERNAL);
  // set up the display and print the version
  pinMode(controllPin, OUTPUT); 
  lcd.begin(WIDTH, 2);
  lcd.clear();
  lcd.print("Temp reader");
  lcd.setCursor(0,1);
  lcd.print("v1.00");
  delay(1000);
  /*lcd.clear();
  lcd.print("powered by XIG");
  lcd.setCursor(0,1);
  lcd.print("->faludi.com/xig");
  delay(2000);*/
  // set the data rate for the NewSoftSerial port,
  //  can be slow when only small amounts of data are being returned
  Serial3.begin(9600);
  xbee.setSerial(Serial3);
}


void loop() {
  unsigned long currentMillis = millis();
  if (vrunl == 0) {
    startArdMillis = currentMillis;
    vrunl = 1;
  } else if (vrunl == 1) {
    if (currentMillis - startArdMillis > nextRunl ){
      vrunl = 2;
    }
  }
   
  // prepare to load some text
  float temperature = tempReading(temperaturePin);  //getting the voltage reading from the temperature sensor
  temperature = (temperature - .5) * 100;          //converting from 10 mv per degree wit 500 mV offset
   
                                                 //to degrees ((volatge - 500mV) times 100)
  tempValue = (int) (temperature*100);
  
  float boiler = getVoltage(boilerPin);
  boilerValue = (int) (boiler*100);
  
  float sensorCalcValue = (round(((analogRead(sensorPin)/5.0)+100.0)/10.0*2.0))/2.0*10.0;
  sensorValue = (int) sensorCalcValue;
  
  float thermistor = thermistorRead(thermistorPin,10000,10000,25,3950);
  thermistorValue = (int) (thermistor*100);
  
  float thermistor2 = thermistorRead(thermistor2Pin,100000,100000,25,4261);
  thermistor2Value = (int) (thermistor2*100);
  
 // sensorValue = sendp * 10;
   avgNumSamp = avgNumSamp + 1;
   if(avgNumSamp > avgMaxSamp) {
     avgNumSamp = 1;
     avgTotalTemp = 0;
   } 
  avgTotalTemp = avgTotalTemp + thermistorValue;
  if(avgNumSamp == avgMaxSamp){
    avgTemp = (int) round(avgTotalTemp/avgNumSamp);
    startup = 1; 
  }
  
  //resxbee
 // vstatus = 2;
   //memset(vpayload, 0, sizeof(vpayload));
    // sprintf(vpayload,"%s","test");
 
    // sprintf(vpayload,"%s","test");
    xbee.readPacket();
   
   if (xbee.getResponse().isAvailable()) {
           
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
  
         chour[0] = rx.getData()[0];
         chour[1] = rx.getData()[1];
         chour[2] = 0;
         vhour = atoi(&chour[0]); 
         cminute[0] = rx.getData()[2];
         cminute[1] = rx.getData()[3];
         cminute[2] = 0;
         vminute = atoi(&cminute[0]);
         cboiler[0] = rx.getData()[4];
         cboiler[1] = 0;
         rvboiler = atoi(&cboiler[0]);
         crtemp[0] = rx.getData()[5];
         crtemp[1] = rx.getData()[6];
         crtemp[2] = rx.getData()[7];
         crtemp[3] = rx.getData()[8];
         crtemp[4] = 0;
         rvtemp = atoi(&crtemp[0]);
         cvchecksum[0] = rx.getData()[9];
         cvchecksum[1] = rx.getData()[10];
         cvchecksum[2] = rx.getData()[11];
         cvchecksum[3] = rx.getData()[12];
         cvchecksum[4] = 0;
         rvchecksum = atoi(&cvchecksum[0]);
         
         mesAvailable = 1; 
               
      }
    } 
    
   if (mesAvailable == 1 && (vhour+vminute+rvboiler+rvtemp) == rvchecksum ){
      rcycle = 0;
      mesAvailable = 0;
   } else {
     rcycle = rcycle+1; 
     mesAvailable = 0; 
   }
   
   if (rcycle > 3000) {
     rcycle = 3000;
   }

   if (rcycle < maxrcycle) {
     remotemode = 1; 
   } else {
     remotemode = 0;
   }
   
  if (remotemode == 1) {   
     avgTemp = rvtemp;
   }
  
  if(currentMillis - previousMillisTemp > intervaltemp) {  
    previousMillisTemp = currentMillis;  
    tempDisValue = (int) round(avgTemp/10);
    
    /*sprintf(texttemp,"%s%i","Act temp: ",tempDisValue);*/
  }

  //sensorValue = vrunl*10;
  
//  delay(1000);
  if(currentMillis - previousMillisDis > intervaldis) {
    previousMillisDis = currentMillis;  
    if (sensorValue != sensorOldValue || tempDisValue != tempDisOldValue || remotemode != remotemodeold || vminute != vminuteold) {
      dtostrf(sensorValue / 10.0,4,1,temstr);
      sprintf(textset,"%s%s%s%i%s%i","Set: ",temstr,"  ",vhour,":",vminute);
      dtostrf(tempDisValue / 10.0,4,1,tmvstr);
      if (remotemode == 1) {
        sprintf(texttemp,"%s%s%s","Act: ",tmvstr," (R)");
      } else {
        sprintf(texttemp,"%s%s%s","Act: ",tmvstr," (L)");
      }
      lcd.clear();
      lcd.print(textset);
      lcd.setCursor(0,1);
      lcd.print(texttemp);
      sensorOldValue = sensorValue;
      tempDisOldValue = tempDisValue;
      remotemodeold = remotemode;
      vminuteold = vminute;
    } 
   }
 
  if(currentMillis - previousMillisSend > intervalSend) {  
    previousMillisSend = currentMillis;
    
    
       if(sendp==0) {
        fxbesend("A02_",sensorValue);
      } else if(sendp==1) {
        fxbesend("A03_",boilerValue);
      } else if(sendp==2) {
        fxbesend("A01_",tempValue); 
      } else if(sendp==3) {
        fxbesend("A05_",thermistor2Value); 
      } else if(sendp==4) {
        fxbesend("A07_",boilerStat); 
      } else if(sendp==5) {
        sprintf(lpayload,"%s%i",lpayload,thermistorValue);
         fxbesendl("A04_",lpayload); 
      }
      
      if(sendp<5) {
        if (sendp ==0 ) {
        sprintf(lpayload,"%i%s",thermistorValue,","); 
        } else {  
        sprintf(lpayload,"%s%i%s",lpayload,thermistorValue,",");  
        }
        sendp = sendp + 1;
      } else {
        sendp = 0;
        memset(lpayload, 0, 50);
      }
    
   }
   
  //  if(currentMillis - previousMillisBlink > intervalBlink) {  
   // previousMillisBlink = currentMillis;
    //  if (runScen == 0) {
    //   digitalWrite(controllPin, HIGH);
    //   runScen = 1;
    //  } else {
    //    digitalWrite(controllPin, LOW);
    //    runScen = 0;
    //  }
  //  }
    
    if(runScen == 0) { //no scenario running
    
      if(startup > 0 && vrunl ==2) {
        if (remotemode == 1) {
          if (rvboiler == 1 ) {
            digitalWrite(controllPin, HIGH);
          } else {
            digitalWrite(controllPin, LOW);
          }
          
        } else {
          runScen = frunScen(avgTemp,sensorValue);
        }
      }
        
    } else if (runScen == 1) { //start scenario
      startScen = currentMillis;
      scenLength = fscenLength(avgTemp,sensorValue);
      runScen = 2;
      boilerStat = 1;
       
      unsigned long scenLengthCalc = (scenLength/(1000UL));
      int sendScenLength = (int) scenLengthCalc;
      delay(50);
      fxbesend("A06_",sendScenLength);
      delay(50);
      
    } else if (runScen == 2) {
      if (boilerStat == 1 ) {
        boilerStat = fboilerStat(startScen,scenLength,currentMillis,avgTemp,sensorValue);
      }
      
      if (boilerStat == 1 ) {
        digitalWrite(controllPin, HIGH);
      } else {
        digitalWrite(controllPin, LOW);
      }
      
      //if (boilerStat == 2 ) {
      //  overshoot = 1;
     // }
      
      if(currentMillis - startScen > maxScen) {
        runScen = 0;
      }
    }
 
delay(10);
} 

float getVoltage(int pin){
   return (analogRead(pin) * .004882814); //converting from a 0 to 1023 digital range
                                        // to 0 to 5 volts (each 1 reading equals ~ 5 millivolts
 }  
  
float tempReading(int pin){
  int tempRead = analogRead(pin);
  float voltage = tempRead * aref_voltage;
  voltage /= 1024.0;
  return voltage;
}

void fxbesend(char fport[4],int fvalue ) {
  sprintf(sendValue,"%s%i",fport,fvalue);
  ZBTxRequest zbtxt = ZBTxRequest(addr64, (uint8_t *)sendValue, strlen(sendValue));
  xbee.send(zbtxt);
}

void fxbesendl(char fport[4],char fpayl[60] ) {
  sprintf(sendValue,"%s%s",fport,fpayl);
  ZBTxRequest zbtxt = ZBTxRequest(addr64, (uint8_t *)sendValue, strlen(sendValue));
  xbee.send(zbtxt);
}


float thermistorRead(int pin, int seriesr, int thermistnominal,int tempnom, int bcof) {
  float tempRead = analogRead(pin);
 
  // convert the value to resistance
  tempRead = 1023 / tempRead - 1;
  tempRead = seriesr / tempRead;
 
  float steinhart;
  steinhart = tempRead / thermistnominal;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= bcof;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (tempnom + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
 
  return steinhart;
}

int frunScen (int actTemp, int setTemp) {
  if ( (setTemp*10) - actTemp > 35) {
     return 1;
  } else {
    return 0;
  }
}

unsigned long fscenLength(int actTemp, int setTemp) {
  unsigned long scnel;
 // int iscnel = 0;
  if ( (setTemp*10) - actTemp > 260) {
     scnel = (6UL*60UL*1000UL);
  } else if ( (setTemp*10) - actTemp > 160) {
    scnel = (5UL*60UL*1000UL);
  } else if ( (setTemp*10) - actTemp > 70) {
    scnel = (4UL*60UL*1000UL);
  } else if ( (setTemp*10) - actTemp > 40) {
    scnel = (3UL*60UL*1000UL);
  } else {
   scnel =  (2UL*60UL*1000UL);
  }
  return scnel;
}

int fboilerStat(unsigned long starts,unsigned long scenl,unsigned long cur ,int actTemp, int setTemp) {
  
  if (actTemp -  (setTemp*10) < 35){ 
    if (cur - starts < scenl) {
      return 1;
    } else {
     return 0;  
    }
      
  } else {
    return 2;
  }
  
}


