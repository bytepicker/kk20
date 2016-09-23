#include <math.h>

#include <KK2LCD.h>
#include <Wire.h>
#include <EEPROM.h>

/* setting measurement period 
 EEMPROM is 1024 long, each entry requires 6 bytes
 so, measuring every minute can be done ~170 times or for ~ 3 hours */

#define SECOND 1000L
#define MINUTE (60*SECOND)
#define HOUR (60*MINUTE)
#define DAY (24*HOUR)

/* set temperature and humidity limit */
const int TEMP_LIMIT = 23;
const int HUM_LIMIT = 50;

/* message buffers */
char mBuf[7];
String Str;

int newBegin = 0;

/* total measurements */
int totalMeas;

/* EEPROM */
const int EEPROM_ADDRESS = 0;

/* SHT21 sensor */
const byte TEMPERATURE = 0xe3;
const byte HUMIDITY = 0xe5;
const byte INVALID_DATA = 0;
const int SHT21_ADDRESS = 0x40;

/* DS1307 clock */
const int DS1307_ADDRESS = 0x68;

/* kk2 pinouts */
const byte SDA_pin = 9;  // PC1 (SDA/PCINT17)  //I2C
const byte SCL_pin = 10; // PC0 (SCL/PCINT16)  //I2C
const byte GND_pin = 11;
const byte VIN_pin = 12;

/* diod */
const byte RED_LED = 13; // PB3 (PCINT11/OC0A/AIN1)

/* buttons */
const byte BUT1 = 14;  //PB7 (PCINT15/OC3B/SCK)    PWM
const byte BUT2 = 15;  //PB6 (PCINT14/OC3A/MISO)   PWM
const byte BUT3 = 16;  //PB5 (PCINT13/ICP3/MOSI)
const byte BUT4 = 17;  //PB4 (PCINT12/OC0B/SS)

/* buzzer */
const byte _BUZZER = 18; // PB1 (PCINT9/CLKO/T1)   CLOCK output can adjust with

/* initial setup */
void setup(){
  pinMode(RED_LED, OUTPUT);

  pinMode(BUT1, INPUT);
  digitalWrite(BUT1, HIGH);
  pinMode(BUT2, INPUT);
  digitalWrite(BUT2, HIGH);
  pinMode(BUT3, INPUT);
  digitalWrite(BUT3, HIGH);
  pinMode(BUT4, INPUT);
  digitalWrite(BUT4, HIGH);

  analogReference(EXTERNAL); // important!!

  st7565Init(Font5x7);
  st7565SetBrightness(12);
  st7565WriteLogo();

  // set power pins
  pinMode(VIN_pin, OUTPUT);
  digitalWrite(VIN_pin, HIGH);
  pinMode(GND_pin, OUTPUT);
  digitalWrite(GND_pin, LOW); 

  // beep and start
  tone(_BUZZER, 100, 10);
}

/* clock */
int getTime(int tVal){
  Wire.begin();
  Wire.beginTransmission(DS1307_ADDRESS);
  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  int reading[7];
  reading[0] = bcdToDec(Wire.read());  // secs
  reading[1] = bcdToDec(Wire.read());  // mins
  reading[2] = bcdToDec(Wire.read() & 0b111111); //24 hour time
  reading[3] = bcdToDec(Wire.read()); //0-6 -> Sunday - Saturday
  reading[4] = bcdToDec(Wire.read());
  reading[5] = bcdToDec(Wire.read());
  reading[6] = bcdToDec(Wire.read());

  return reading[tVal];
}


byte bcdToDec(byte val)  {
  // convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

/* temperature & humidity */
float getMeasurement(byte type)
{
  Wire.begin();
  Wire.beginTransmission(SHT21_ADDRESS);   // transmit to device 0x40 (SHT21)
  Wire.write(type);               // Trigger measurement based on the given type
  Wire.endTransmission();

  Wire.requestFrom(SHT21_ADDRESS, 3);      // request 3 bytes from slave device

  if (3 == Wire.available()) {    // if three  bytes were received
    byte reading[3];  
    reading[0] = Wire.read();     // 1st part of the measurement
    reading[1] = Wire.read();     // 2nd part of the measurement
    reading[2] = Wire.read();     // CRC checksum

    if(isValidateCRC(reading, 2, reading[2])){

      // put the effective measured value together
      int ticks = reading[0] << 8;
      ticks |= reading[1];

      // Magic numbers
      if(type == TEMPERATURE)
        return (-46.85 + 175.72 / 65536.0 * ticks);
      if(type == HUMIDITY)
        return (-6.0 + 125.0 / 65536.0 * ticks);
      else
        return INVALID_DATA;
    }
  }
  return INVALID_DATA;
}

byte isValidateCRC(byte data[], byte nbrOfBytes, byte checksum){
  byte crc = 0;
  byte byteCtr;

  for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr) {
    crc ^= (data[byteCtr]);
    for (byte bit = 8; bit > 0; --bit) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x131;
      else
        crc = (crc << 1);
    }
  }
  return crc == checksum;
}

/* print logs */
void printLog(int beg, int end){
  int j = 0;
  for(int i=beg; i<=end; ++i){
    sprintf(mBuf, "%u", EEPROM.read(0 + i*6));
    st7565DrawString(0*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(3*4, 10*(j+1) + 6, PSTR(":"));
    
    sprintf(mBuf, "%u", EEPROM.read(1 + i*6));
    st7565DrawString(4*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(6*4, 10*(j+1) + 6, PSTR("  ")); 
    
    sprintf(mBuf, "%u", EEPROM.read(2 + i*6));
    st7565DrawString(9*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(12*4, 10*(j+1) + 6, PSTR("."));
    
    sprintf(mBuf, "%u", EEPROM.read(3 + i*6));
    st7565DrawString(13*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(16*4, 10*(j+1) + 6, PSTR("C, "));
    
    sprintf(mBuf, "%u", EEPROM.read(4 + i*6));
    st7565DrawString(20*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(23*4, 10*(j+1) + 6, PSTR("."));
    
    sprintf(mBuf, "%u", EEPROM.read(5 + i*6));
    st7565DrawString(24*4, 10*(j+1) + 6, mBuf);
    
    st7565DrawString_P(27*4, 10*(j+1) + 6, PSTR("%"));
    j++;
    }
  }

/* button 4 hook */
byte button4Pressed(){
  if(!digitalRead(BUT4))
  {
    delayMicroseconds(500);
    if(!digitalRead(BUT4))
    {
      while(!digitalRead(BUT4))
      {
        st7565SetBrightness(12);
        st7565ClearBuffer();
        st7565SetFont( Font12x16 );
        st7565DrawString_P( 42, 26 ,  PSTR("Next") );
        st7565Refresh();
        digitalWrite(RED_LED,HIGH);
      }
      delayMicroseconds(100);
      digitalWrite(RED_LED,LOW);    
      return 1;
    }
  }
  return 0;
}

/* current data and time */
void monitor(){
  st7565SetBrightness(12);
  st7565ClearBuffer();

  while (true)
  {
    float temp, hum;

    st7565SetBrightness(12);
    st7565ClearBuffer();
    st7565SetFont(Font12x16);
    st7565DrawString_P(0, 0, PSTR("Monitoring"));
    st7565SetFont(Font5x7); 

    // hours
    sprintf(mBuf, "%u", getTime(2));
    st7565DrawString( 0, 56, mBuf);
    st7565DrawString_P( 13, 56, PSTR(":") );

    // minuits
    sprintf(mBuf, "%u", getTime(1));
    st7565DrawString( 19, 56, mBuf);

    delayMicroseconds(10);
    Str.toCharArray(mBuf, 6);
    st7565DrawString_P(0, 26, PSTR("TEMPERATURE: "));
    st7565DrawString(13*6, 26, dtostrf(getMeasurement(TEMPERATURE), 6, 2, mBuf));

    delayMicroseconds(10);
    Str.toCharArray(mBuf, 6);
    st7565DrawString_P(0, 36, PSTR("HUMIDITY: "));

    /* humidity sensor fails sometimes */
    if (hum < 0){
      st7565DrawString(13*6, 36, "  N/A");
      for(int i=0; i<2; ++i){
        tone(_BUZZER, 100, 10);
      }
    }
    else {
      st7565DrawString(13*6, 36, dtostrf(getMeasurement(HUMIDITY), 6, 2, mBuf));
    }

    delay(1000);

    st7565DrawString_P( 102, 56, PSTR("Exit") );
    st7565Refresh(); 

    if(button4Pressed())    
      return;    
  }
}

/* set data logging */
void logData()
{
  st7565SetBrightness(12);
  st7565ClearBuffer();

  int hrs = 0, mins = 0, secs = 0, done = 0;

  while(true){
    st7565ClearBuffer();
    st7565SetFont( Font12x16 );
    st7565DrawString_P( 0, 0,  PSTR("Log data") );

    st7565SetFont(Font5x7); 
    st7565DrawString_P(0, 16, PSTR("Default: 1/minute"));

    // buttons
    st7565SetFont( Font12x24Numbers );

    /* get and display hours */
    sprintf(mBuf, "%u", hrs);
    st7565DrawString( 38, 38, mBuf);
    st7565SetFont( Font12x16 );
    st7565DrawString_P( 65, 45, PSTR(":") );

    if(!digitalRead(BUT2))
      delayMicroseconds(100);
    if(!digitalRead(BUT2))
      hrs++;
    delay(100);

    // limiter
    if(mins > 59){
      mins = 0;
      hrs++;
    }

    /* get and display minutes */
    st7565SetFont( Font12x24Numbers );
    sprintf(mBuf, "%u", mins);
    st7565DrawString( 76, 38, mBuf);

    if(!digitalRead(BUT3))
      delayMicroseconds(100);
    if(!digitalRead(BUT3))
      mins++;
    delay(100);

    /* start measurements and store data to EEPROM */
    if(!digitalRead(BUT1)){     
      // beep to start
      tone(_BUZZER, 100, 10);

      // we need to 'split' our floats
      double intTemp, intHum, flTemp, flHum;

      // total measurements required
      totalMeas = hrs * 60 + mins;

      for(int i=0; i<totalMeas; ++i) {
        digitalWrite(RED_LED,HIGH);

        EEPROM.write(i *6, getTime(2));    
        EEPROM.write(1 + i*6, getTime(1));

        flTemp = modf(getMeasurement(TEMPERATURE), &intTemp);
        EEPROM.write(2 + i*6, intTemp);

        flTemp *= 100;
        EEPROM.write(3 + i*6, flTemp);

        flHum = modf(getMeasurement(HUMIDITY), &intHum);
        EEPROM.write(4 + i*6, intHum);

        flHum *= 100;
        EEPROM.write(5 + i*6, flHum);

        // important!
        delay(SECOND);
      }

      // bye
      digitalWrite(RED_LED,LOW);
      done +=1;
      tone(_BUZZER, 100, 10);
    }

    st7565SetFont( Font5x7 );
    st7565DrawString_P( 102, 56, PSTR("Exit") );
    st7565DrawString_P( 0, 56, PSTR("Start") );

    if(done){
      st7565DrawString_P(0, 36, PSTR("DONE!"));     
      mins = 0;
      hrs = 0;
    }
    st7565Refresh();

    if(button4Pressed())    
      return;
  }
}

void logEvents(){
  st7565SetBrightness(12);
  st7565ClearBuffer();

  while(true){
    st7565ClearBuffer();
    st7565SetFont(Font12x16);
    st7565DrawString_P(0, 0, PSTR("Log events"));

    st7565SetFont(Font5x7); 

    float currentTemp = getMeasurement(TEMPERATURE);
    float currentHum = getMeasurement(HUMIDITY);

    // show limits
    st7565DrawString_P(0, 26, PSTR("TEMP LIMIT: "));
    sprintf(mBuf, "%u", TEMP_LIMIT);
    st7565DrawString(13*6, 26, mBuf);
    st7565DrawString_P(13*7, 26, PSTR(" C"));

    if(currentTemp > TEMP_LIMIT){
      tone(_BUZZER, 100, 10);
      digitalWrite(RED_LED,HIGH);
      // write time to EEPROM from 0
      for(int i=2; i>0; i--){
        EEPROM.write(EEPROM_ADDRESS, getTime(i));
        //EEPROM_ADDRESS++;
      }
      // write value to EEPROM
      EEPROM.write(EEPROM_ADDRESS, currentTemp);
      //EEPROM_ADDRESS++;
      digitalWrite(RED_LED,LOW);
    }

    st7565DrawString_P(0, 36, PSTR("HUM LIMIT: "));
    sprintf(mBuf, "%u", HUM_LIMIT);
    st7565DrawString(13*6, 36, mBuf);
    st7565DrawString_P(13*7, 36, PSTR(" %"));

    if(currentTemp > HUM_LIMIT){
      tone(_BUZZER, 100, 10);
      digitalWrite(RED_LED,HIGH);
      // write time to eeprom

      digitalWrite(RED_LED,LOW);
    }

    st7565SetFont( Font5x7 );
    st7565DrawString_P( 102, 56, PSTR("Exit") );

    st7565Refresh(); 

    if(button4Pressed())    
      return;    
  }
}

///* testing endTime */
//void testTime()
//{
//  st7565SetBrightness(12);
//  st7565ClearBuffer();
//
//  int hrs = 0, mins = 0, secs = 0, done = 0;
//
//  while(true){
//    st7565ClearBuffer();
//    st7565SetFont( Font12x16 );
//    st7565DrawString_P( 0, 0,  PSTR("Test timing") );
//
//    // buttons
//    st7565SetFont( Font12x24Numbers );
//
//    /* get and display hours */
//    sprintf(mBuf, "%u", hrs);
//    st7565DrawString( 38, 38, mBuf);
//    st7565SetFont( Font12x16 );
//    st7565DrawString_P( 65, 45, PSTR(":") );
//
//    if(!digitalRead(BUT2))
//      delayMicroseconds(100);
//    if(!digitalRead(BUT2))
//      hrs++;
//    delay(100);
//
//    // max 2 hours 50 mins
//    if(hrs>2){
//      hrs = 0;
//    }
//    if(mins > 59){
//      mins = 0;
//    }
//
//    /* get and display minutes */
//    st7565SetFont( Font12x24Numbers );
//    sprintf(mBuf, "%u", mins);
//    st7565DrawString( 76, 38, mBuf);
//
//    // now
//    int nowSecs = getTime(0);
//    int nowMins = getTime(1);
//    int nowHrs = getTime(2);
//
//    st7565SetFont(Font5x7); 
//    sprintf(mBuf, "%u", nowHrs);
//    st7565DrawString(0, 26, mBuf);
//
//    st7565SetFont(Font5x7); 
//    sprintf(mBuf, "%u", nowMins);
//    st7565DrawString(6*6, 26, mBuf);
//
//    st7565SetFont(Font5x7); 
//    sprintf(mBuf, "%u", nowSecs);
//    st7565DrawString(13*6, 26, mBuf);
//
//    // count end time
//    int endHrs = nowHrs + hrs;
//    int endMins = nowMins + mins;
//    int endSecs;
//    if(nowSecs == 0){
//      endSecs = 59;
//    }
//    else{
//      endSecs = nowSecs - 1;
//    }
//
//    if(endMins >= 60){
//      endHrs++;
//      endMins -= 60;
//    }
//
//    st7565SetFont(Font5x7); 
//    sprintf(mBuf, "%u", endHrs);
//    st7565DrawString(0, 36, mBuf);
//    sprintf(mBuf, "%u", endMins);
//    st7565DrawString(6*6, 36, mBuf);
//    sprintf(mBuf, "%u", endSecs);
//    st7565DrawString(13*6, 36, mBuf);
//
//    if(!digitalRead(BUT3))
//      delayMicroseconds(100);
//    if(!digitalRead(BUT3))
//      mins++;
//    delay(100);
//
//    st7565SetFont( Font5x7 );
//    st7565DrawString_P( 102, 56, PSTR("Exit") );
//    st7565DrawString_P( 0, 56, PSTR("Start") );
//    st7565Refresh();
//
//    if(button4Pressed())    
//      return;
//  }
//}

/* display EEPROM */
void viewLog(){
  st7565SetBrightness(12);
  st7565ClearBuffer();
  
   
  while(true){

    st7565ClearBuffer();
    st7565SetFont(Font12x16);
    st7565DrawString_P(0, 0, PSTR("View log"));

    st7565SetFont( Font5x7 );
    
//    printLog(newBegin, newBegin+3);
    // draw an arrow
    if (totalMeas > 4){
      st7565DrawString_P( 80, 56, PSTR("-->") );
      
        printLog(newBegin, newBegin+3);
//        if(!digitalRead(BUT3)){
          delayMicroseconds(100);
          newBegin += 4;
//        }
    }
    else{
      printLog(0, totalMeas-1);
    }
    
    
    // erasing all data
    if(!digitalRead(BUT1)){
      delayMicroseconds(100);
      tone(_BUZZER, 100, 100);
      for(int i=0; i<1024; ++i){
        EEPROM.write(i, 0);
      }
    }
    
    st7565DrawString_P( 102, 56, PSTR("Exit") );
    st7565DrawString_P( 0, 56, PSTR("Erase") );

    st7565Refresh();
    if(button4Pressed())    
      return;
    }
}
//}

void loop() {
  //monitor();    // current data
  //logEvents();  // log events time 
  logData();   // store data every minute for N times
  //testTime();
  viewLog();
}













