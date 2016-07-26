#include <KK2LCD.h>

const byte IN1 = 0; // PD3 (PCINT27/TXD1/INT1)  not tested, but use Serial1
const byte IN2 =
    1; // PD2 (PCINT26/RXD1/INT0)  interrupts good for CCPM decoding.
const byte IN3 = 2; // PD0 (PCINT24/RXD0/T3)  tx0 is on the lcd not sure if
                    // using this would conflict with the lcd
const byte IN4 = 3; // PB2 (PCINT10/INT2/AIN0)
const byte IN5 = 4; // PB0 (PCINT8/XCK0/T0)  //timer/counter0 source

// motor outputs can also be digital inputs. these also have PCINT16 to 23
// Arduino interrupts not tested.
const byte OUT1 = 5;  // PC6 (TOSC1/PCINT22)   //32.768kHz crystal or custom
                      // clock source for counter (rpm sensor)
const byte OUT2 = 6;  // PC4 (TDO/PCINT20)   //JTAG
const byte OUT3 = 7;  // PC2 (TCK/PCINT18)   //JTAG
const byte OUT4 = 8;  // PC3 (TMS/PCINT19)  //JTAG
const byte OUT5 = 9;  // PC1 (SDA/PCINT17)  //I2C      i2c not tested
const byte OUT6 = 10; // PC0 (SCL/PCINT16)  //I2C
const byte OUT7 = 11; // PC5 (TDI/PCINT21)   //JTAG
const byte OUT8 = 12; // PC7 (TOSC2/PCINT23)   //32.768kHz crystal

const byte RED_LED = 13; // PB3 (PCINT11/OC0A/AIN1)  //same as arduino!

// important enable the internal pullups when using these as inputs
const byte BUT1 = 14; // PB7 (PCINT15/OC3B/SCK)    PWM     pwm not tested
const byte BUT2 = 15; // PB6 (PCINT14/OC3A/MISO)   PWM
const byte BUT3 = 16; // PB5 (PCINT13/ICP3/MOSI)
const byte BUT4 = 17; // PB4 (PCINT12/OC0B/SS)

const byte _BUZZER = 18; // PB1 (PCINT9/CLKO/T1)   CLOCK output can adjust with
                         // system prescaler. (make tones) not tested

// analog reads must be done using thier channels, specifying digital pin
// numbers will not work in this case
const byte BATT = 3;

const byte GYR_R = 1;
const byte GYR_Y = 2;
const byte GYR_P = 4;

const byte ACC_X = 5;
const byte ACC_Y = 6;
const byte ACC_Z = 7;

/* initial setup */
void setup() {
  pinMode(RED_LED, OUTPUT);

  pinMode(GYR_R, INPUT);
  pinMode(GYR_Y, INPUT);
  pinMode(GYR_P, INPUT);

  pinMode(ACC_X, INPUT);
  pinMode(ACC_Y, INPUT);
  pinMode(ACC_Z, INPUT);

  pinMode(BUT1, INPUT);
  digitalWrite(BUT1, HIGH); // enable internal pullup.

  pinMode(BUT2, INPUT);
  digitalWrite(BUT2, HIGH);

  pinMode(BUT3, INPUT);
  digitalWrite(BUT3, HIGH);

  pinMode(BUT4, INPUT);
  digitalWrite(BUT4, HIGH);

  analogReference(EXTERNAL); // important!!

  st7565Init(Font5x7);
  st7565SetBrightness(12);
  st7565DrawString_P(35, 40, PSTR("Arduino on"));
  st7565DrawString_P(35, 32, PSTR(" the KK2."));
  st7565DrawString_P(35, 24, PSTR("Test suite"));
  st7565WriteLogo(); // see library to modify

  // autoload screen after 2s
  size_t delayTime = 2000;
  delay(delayTime);
  //  beep
  tone(_BUZZER, 100, 10);
}

/* button 4 hook */
byte button4Pressed() {
  if (!digitalRead(BUT4)) {
    delayMicroseconds(500);
    if (!digitalRead(BUT4)) {
      while (!digitalRead(BUT4)) {
        st7565SetBrightness(12);
        st7565ClearBuffer();
        st7565SetFont(Font12x16);
        st7565DrawString_P(42, 26, PSTR("Next"));
        st7565Refresh();
        digitalWrite(RED_LED, HIGH);
      }
      delayMicroseconds(100);
      digitalWrite(RED_LED, LOW);
      tone(_BUZZER, 100, 10);
      return 1;
    }
  }
  return 0;
}

String Str;
char str[7];

/* get analog gyroscope data */
void gyro() {
  int aread = 0;
  st7565SetBrightness(12);
  while (true) {
    st7565ClearBuffer();
    st7565SetFont(Font12x16);
    st7565DrawString_P(0, 0, PSTR("Gyroscope"));
    st7565SetFont(Font5x7);

    delayMicroseconds(10);
    aread = analogRead(GYR_R);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 32, PSTR("GYR_R "));
    st7565DrawString(16 * 6, 16, str);

    delayMicroseconds(10);
    aread = analogRead(GYR_Y);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 16, PSTR("GYR_Y "));
    st7565DrawString(16 * 6, 24, str);

    delayMicroseconds(10);
    aread = analogRead(GYR_P);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 24, PSTR("GYR_P "));
    st7565DrawString(16 * 6, 32, str);

    st7565DrawString_P(102, 56, PSTR("Next"));
    st7565Refresh();

    delayMicroseconds(500);
    if (!digitalRead(BUT4)) {
      while (!digitalRead(BUT4)) {
        st7565SetBrightness(12);
        st7565ClearBuffer();
        st7565SetFont(Font12x16);
        st7565DrawString_P(42, 26, PSTR("Next"));
        st7565Refresh();

        if (button4Pressed())
          return;
      }
    }
  }
}

/* get analog accelerometer data */
void accel() {
  int aread = 0;
  st7565SetBrightness(12);
  while (true) {
    st7565ClearBuffer();
    st7565SetFont(Font12x16);
    st7565DrawString_P(0, 0, PSTR("Accelerate"));
    st7565SetFont(Font5x7);

    delayMicroseconds(10);
    aread = analogRead(ACC_X);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 16, PSTR("ACC_X "));
    st7565DrawString(6 * 6, 16, str);

    delayMicroseconds(10);
    aread = analogRead(ACC_Y);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 24, PSTR("ACC_Y "));
    st7565DrawString(6 * 6, 24, str);

    delayMicroseconds(10);
    aread = analogRead(ACC_Z);
    Str = String(aread);
    Str.toCharArray(str, 6);
    st7565DrawString_P(0, 32, PSTR("ACC_Z "));
    st7565DrawString(6 * 6, 32, str);

    st7565DrawString_P(102, 56, PSTR("Next"));
    st7565Refresh();

    delayMicroseconds(500);
    if (!digitalRead(BUT4)) {
      while (!digitalRead(BUT4)) {
        st7565SetBrightness(12);
        st7565ClearBuffer();
        st7565SetFont(Font12x16);
        
        st7565DrawString_P(42, 26, PSTR("Next"));
        st7565Refresh();
        digitalWrite(RED_LED, HIGH);
        
        if (button4Pressed())
          return;
      }
    }
  }
}

void loop() {
  gyro();
  accel();
}

