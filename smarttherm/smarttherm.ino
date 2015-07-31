/*
 * ********* Smart Thermostat - Arduino ********
 * by Niek Temme http://niektemme.com
 *
 * smart thermostat - arduino part
 * Documentation: http://niektemme.com/2015/07/31/smart-thermostat/ @@to do
 * 
 */
#include <XBee.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

// create a software serial port for the XBee
SoftwareSerial Serial3(6, 7);

//XBee objects and constants
XBee xbee = XBee();
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40B7FB33); //XBee adress of XBee connected to Raspberry PI
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();

// LCD Connection & screen size
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define WIDTH 16

//ref voltage
#define aref_voltage 3.3       

//variables for settemp variable resistor
int sensorPin = 0; //sensor = settemp
int sensorValue = 0;
int sensorOldValue = 0;

//variables for TMP36
int temperaturePin = 1; //act temp
int tempValue = 0;
int tempOldValue = 0;
int tempDisValue = 0;
int tempDisOldValue = 0;

//variables for boiler on off sensor (photo resistor)
int boilerPin = 2; 
int boilerValue = 0;

//variables for thermistor
int thermistorPin= 3;
int thermistorValue =0;
int thermistor2Pin= 4;
int thermistor2Value =0;

//char varaibles for XBseend
char sendValue[10];

//char viarables for LCD display
char textset[24];
char texttemp[20];
char temstr[4];
char tmvstr[4];

//variables for calculating avarage temperature at Arduino
int avgTotalTemp = 0;
int avgTemp = 0;
int avgNumSamp = 0;
int avgMaxSamp =10;

//variable for carousell deciding which sensor value to send over xbee to raspberry pi
int sendp = 0;

//variables for seperatere time intervals between actions
long previousMillisDis = 0; //display update
long previousMillisTemp = 0; //temperature update at display 
long intervaldis = 100; //display update 
long intervaltemp = 2000; //temperature update at display 
long previousMillisSend = 0; //previous send
long intervalSend = 100; //interval between xbee sends

//controll variables
int startup = 0; //initial delay before temparature is set
int controllPin = 9; //pin connected to relay
int runScen = 0; //temperature run level 0 ,1 or 2
unsigned long startScen = 0; //scenario started
unsigned long scenLength = 0; //minutes on
unsigned long maxScen = 600000; //10 minutes
int boilerStat = 0; //boiler status

//variables for receiving xbee 
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

//timer to check if raspberry pi is still sending messages to arduino
int remotemode = 0;
int remotemodeold = 0;
unsigned int rcycle = 3000;
unsigned int maxrcycle = 2000;

//variables of inital check if arduino has been running long enough
int vrunl = 0; //
unsigned long startArdMillis = 0; //start millis
unsigned long nextRunl = 10000; //milisecond arduino has to be running


void setup() {
   analogReference(EXTERNAL); //analogue refference

  // start display
  pinMode(controllPin, OUTPUT); 
  lcd.begin(WIDTH, 2);
  lcd.clear();
  lcd.print("Smart Therm");
  lcd.setCursor(0,1);
  lcd.print("v1.00");
  delay(1000);
  
  //Start XBee software serial
  Serial3.begin(9600);
  xbee.setSerial(Serial3);
}


void loop() {
  unsigned long currentMillis = millis();
  
  //vrunl is used to see of Arduino has been running for x number of second to make sure a viable actual temperature has been received or measured 
  if (vrunl == 0) {
    startArdMillis = currentMillis;
    vrunl = 1;
  } else if (vrunl == 1) {
    if (currentMillis - startArdMillis > nextRunl ){
      vrunl = 2;
    }
  }
   
  // reading TMP36 value 
  float temperature = tempReading(temperaturePin);  //getting the voltage reading from the temperature sensor
  temperature = (temperature - .5) * 100;          //converting from 10 mv per degree wit 500 mV offset
   //to degrees ((volatge - 500mV) times 100)
  tempValue = (int) (temperature*100);
  
  //reading boiler on of state
  float boiler = getVoltage(boilerPin);
  boilerValue = (int) (boiler*100);
  
  //avaraging set temperature to .5 degrees celcius
  float sensorCalcValue = (round(((analogRead(sensorPin)/5.0)+100.0)/10.0*2.0))/2.0*10.0;
  sensorValue = (int) sensorCalcValue;
  
  //Reading thermistor 1 value using thermistorRead() function, sending nominal value and bcofficient (steinhart) to function
  float thermistor = thermistorRead(thermistorPin,10000,10000,25,3950);
  thermistorValue = (int) (thermistor*100);
  
  //Reading thermistor 2 value using thermistorRead() function, sending nominal value and bcofficient (steinhart) to function 
  float thermistor2 = thermistorRead(thermistor2Pin,100000,100000,25,4261);
  thermistor2Value = (int) (thermistor2*100);
  
 // calculating avarage temperature using thermistor 1 value
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
  
 
 //reading xbee message
   xbee.readPacket();
   
   if (xbee.getResponse().isAvailable()) {
           
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
  
        //convert xbee message to seperate integer values <hour><minute><boiler state><act temperature><checksum>
         chour[0] = rx.getData()[0]; //hour
         chour[1] = rx.getData()[1];
         chour[2] = 0; //0 is used to end array
         vhour = atoi(&chour[0]); //only first position of array is needed to confert string array to int
         cminute[0] = rx.getData()[2]; //minute
         cminute[1] = rx.getData()[3];
         cminute[2] = 0;  //0 is used to end array
         vminute = atoi(&cminute[0]); //only first position of array is needed to confert string array to int
         cboiler[0] = rx.getData()[4]; //boiler state 1 or 0
         cboiler[1] = 0;  //0 is used to end array
         rvboiler = atoi(&cboiler[0]); //only first position of array is needed to confert string array to int
         crtemp[0] = rx.getData()[5]; //actual temperature 
         crtemp[1] = rx.getData()[6];
         crtemp[2] = rx.getData()[7];
         crtemp[3] = rx.getData()[8];
         crtemp[4] = 0;  //0 is used to end array
         rvtemp = atoi(&crtemp[0]); //only first position of array is needed to confert string array to int
         cvchecksum[0] = rx.getData()[9]; //checksum
         cvchecksum[1] = rx.getData()[10];
         cvchecksum[2] = rx.getData()[11];
         cvchecksum[3] = rx.getData()[12];
         cvchecksum[4] = 0;  //0 is used to end array
         rvchecksum = atoi(&cvchecksum[0]); //only first position of array is needed to confert string array to int
         
         mesAvailable = 1; 
               
      }
    } 
    
   //check if correct message is avaliable
   if (mesAvailable == 1 && (vhour+vminute+rvboiler+rvtemp) == rvchecksum ){
      rcycle = 0;
      mesAvailable = 0;
   } else {
     rcycle = rcycle+1; 
     mesAvailable = 0; 
   }
   
   //when larger than 3000 reset tot 3000 to prevent int overflow
   if (rcycle > 3000) {
     rcycle = 3000;
   }

  //check if thermostat should be in local or remote mode
   if (rcycle < maxrcycle) {
     remotemode = 1; 
   } else {
     remotemode = 0;
   }
   
  if (remotemode == 1) {   //if remote mode is true
     avgTemp = rvtemp; //set main actual temperature to the actual temperature send by the raspberry pi 
   }
  
  
  //interval display actual temperature to prevent constant LCD updates
  if(currentMillis - previousMillisTemp > intervaltemp) {  
    previousMillisTemp = currentMillis;  
    tempDisValue = (int) round(avgTemp/10);
    
  }

  
  //update display only if there is something to update. This tor prevent lcd flikering)
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
 
 
 
 //carousell to send one sensor value over xbee each send interval
  if(currentMillis - previousMillisSend > intervalSend) {  
    previousMillisSend = currentMillis;
    
    
      if(sendp==0) {
        fxbesend("A01_",tempValue);  
      } else if(sendp==1) {
        fxbesend("A02_",sensorValue);
      } else if(sendp==2) {
        fxbesend("A03_",boilerValue);
      } else if(sendp==3) {
         fxbesend("A04_",thermistorValue);  
      } else if(sendp==4) {
        fxbesend("A05_",thermistor2Value); 
      } else if(sendp==5) {
        fxbesend("A07_",boilerStat); 
      }
      
      if(sendp<5) {
        sendp = sendp + 1;   
      } else {
        sendp = 0;
      }
    
   }
   

//main boiler controll loop. 
//3 run levels. 
// run level 0 = initial,
// run level 1 = determine how many mintues boier shoudl be on, 
// run level 2 = continuesly check of boier should be on for 10 minutes  

    if(runScen == 0) { //no scenario running
    
      if(startup > 0 && vrunl ==2) { //wait for a time delay to make sure there is correctly measured avarage temperature
      
      //if temotemode is true base boiler on/off on message send by raspberry pi
        if (remotemode == 1) { 
          if (rvboiler == 1 ) {
            digitalWrite(controllPin, HIGH);
          } else {
            digitalWrite(controllPin, LOW);
          }
      
      //when in local mode check if controll should go to runlevel 1    
        } else {
          runScen = frunScen(avgTemp,sensorValue);
        }
      }
        
    } else if (runScen == 1) { //start scenario run level 1
      startScen = currentMillis;
      scenLength = fscenLength(avgTemp,sensorValue); //check how many seconds boiler should be on in 10 minute interval
      runScen = 2;
      boilerStat = 1; //turn boiler on
      
      //sending minutes boiler on time to Rapsberry PI for monitoring purposses
      unsigned long scenLengthCalc = (scenLength/(1000UL));
      int sendScenLength = (int) scenLengthCalc;
      delay(50);
      fxbesend("A06_",sendScenLength); 
      delay(50);
      
      
    } else if (runScen == 2) { //runlevel 2 always runs for 10 minutes
      if (boilerStat == 1 ) { //onley check boiler stat if boiler is on this to prevent on/off fluctuation af ter overshoot 
        boilerStat = fboilerStat(startScen,scenLength,currentMillis,avgTemp,sensorValue); //continuesly check if boiler should be on
      }
      
      if (boilerStat == 1 ) { //turn relay to on of boiler should be on
        digitalWrite(controllPin, HIGH);
      } else {
        digitalWrite(controllPin, LOW);
      }
      
      //after 10 minutes go back to run level 0
      if(currentMillis - startScen > maxScen) {
        runScen = 0;
      }
    }

//small delay. Intervals with millis() are used for larger intervals between functions 
delay(10);
} //end main loop


//function to get analoge value to digital range
float getVoltage(int pin){
   return (analogRead(pin) * .004882814); //converting from a 0 to 1023 digital range
                                        // to 0 to 5 volts (each 1 reading equals ~ 5 millivolts
 }  

//tmp36 sensor fucntion
float tempReading(int pin){
  int tempRead = analogRead(pin);
  float voltage = tempRead * aref_voltage;
  voltage /= 1024.0;
  return voltage;
}

//XBee send function
void fxbesend(char fport[4],int fvalue ) {
  sprintf(sendValue,"%s%i",fport,fvalue);
  ZBTxRequest zbtxt = ZBTxRequest(addr64, (uint8_t *)sendValue, strlen(sendValue));
  xbee.send(zbtxt);
}


//function for read thermistor value given nominal value and bcofficient (steinhart)
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


//function to determine if boiler controll loop should go to run level 1
int frunScen (int actTemp, int setTemp) {
  if ( (setTemp*10) - actTemp > 35) {
     return 1;
  } else {
    return 0;
  }
}

//function to determine how many seconds boiler should go on within 10 minute interval
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


//function to check if boiler should stay on or go off
int fboilerStat(unsigned long starts,unsigned long scenl,unsigned long cur ,int actTemp, int setTemp) {
  
  if (actTemp -  (setTemp*10) < 35){ //criteria 1: only say on if act temperature is below set temperature + margin
    if (cur - starts < scenl) { //criteria 2: only stay on of boiler has not been on for the number of seconds determined by fscenLength
      return 1; //stay on
    } else {
     return 0;  //go off (because boiler was on for number of minues determined by fscenLength
    }
      
  } else {
    return 2; //go off (2 is used to monitor overflow)
  }
  
}


