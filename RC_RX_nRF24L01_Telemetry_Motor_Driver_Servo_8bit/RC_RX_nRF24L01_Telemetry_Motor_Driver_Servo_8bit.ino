
#include <RF24.h>         //https://github.com/nRF24/RF24
#include <nRF24L01.h>
//#include <printf.h>       //print the radio debug info
#include <DigitalIO.h>    //https://github.com/greiman/DigitalIO
#include "ServoTimer2.h"  //used locally https://github.com/nabontra/ServoTimer2
#include "PWMFrequency.h" //used locally https://github.com/TheDIYGuy999/PWMFrequency

// RX battery voltage settings
#define RX_battery_voltage   4.2
#define RX_monitored_voltage 3.3

//free pins
//pin            3
//pin            4
//pin            7
//pin            8
//pin            A5
//pin            A6

//pins for servos
#define serv1    11 //MOSI
#define serv2    12 //MISO
#define serv3    13 //SCK
 
//pwm pins for motor
#define pwm1     5  //MotorA/steering
#define pwm2     6  //MotorA/steering
#define pwm3     9  //MotorB/throttle
#define pwm4     10 //MotorB/throttle

//LED RX battery and RF on/off
#define led      2

//input RX battery
#define inRXbatt A7

//pins for nRF24L01
#define CE       A0 
#define CSN      A1 

//software SPI http://tmrh20.github.io/RF24/Arduino.html
//----- MISO     18 - A4
//----- MOSI     17 - A3
//----- SCK      16 - A2

RF24 radio(CE, CSN); //setup CE and CSN pins

const byte addresses[][6] = {"tx001", "rx002"};

//************************************************************************************************************************************************************************
//this structure defines the received data in bytes (structure size max. 32 bytes) ***************************************************************************************
//************************************************************************************************************************************************************************
struct packet
{
  unsigned int steering;
  unsigned int throttle;
  unsigned int ch3;
  unsigned int ch4;
  unsigned int ch5;
};
packet rc_data; //create a variable with the above structure

//************************************************************************************************************************************************************************
//this struct defines data, which are embedded inside the ACK payload ****************************************************************************************************
//************************************************************************************************************************************************************************
struct ackPayload
{
  float RXbatt;
};
ackPayload payload;

//************************************************************************************************************************************************************************
//reset values ​​(min = 1000us, mid = 1500us, max = 2000us) ****************************************************************************************************************
//************************************************************************************************************************************************************************
void resetData()
{
  rc_data.steering = 1500;
  rc_data.throttle = 1500;
  rc_data.ch3      = 1500;     
  rc_data.ch4      = 1500;
  rc_data.ch5      = 1500;
}

//************************************************************************************************************************************************************************
//create servo object ****************************************************************************************************************************************************
//************************************************************************************************************************************************************************
ServoTimer2 servo1;
ServoTimer2 servo2;
ServoTimer2 servo3;

void attachServoPins()
{
  servo1.attach(serv1);
  servo2.attach(serv2);
  servo3.attach(serv3);
}

int ch3_value = 0, ch4_value = 0, ch5_value = 0;

void outputServo()
{
  servo1.write(ch3_value);   
  servo2.write(ch4_value); 
  servo3.write(ch5_value);

  ch3_value = map(rc_data.ch3, 1000, 2000, 1000, 2000); 
  ch4_value = map(rc_data.ch4, 1000, 2000, 1000, 2000);
  ch5_value = map(rc_data.ch5, 1000, 2000, 1000, 2000); 

//  Serial.println(rc_data.ch3); //print value ​​on a serial monitor 
}

//************************************************************************************************************************************************************************
//setup frequencies and motors control ***********************************************************************************************************************************
//************************************************************************************************************************************************************************
int motA_value = 0, motB_value = 0;

void outputPWM()
{  
/*
 * The base frequency for pins 3, 9, 10, 11 is 31250Hz.
 * The base frequency for pins 5, 6         is 62500Hz.
 * 
 * The divisors available on pins 5, 6, 9, 10 are: 1, 8, 64, 256, and 1024.
 * The divisors available on pins 3, 11       are: 1, 8, 32, 64, 128, 256, and 1024.
 *    
 * Pins 5, 6  are paired on timer0, functions delay(), millis() and micros()
 * D5   pwm 976Hz(default), timer0, 8-bit 
 * D6   pwm 976Hz(default), timer0, 8-bit
 * 
 * Pins 9, 10 are paired on timer1, Servo library
 * D9   pwm 488Hz(default), timer1, 16-bit
 * D10  pwm 488Hz(default), timer1, 16-bit    
 * 
 * Pins 3, 11 are paired on timer2, ServoTimer2 library
 * D3   pwm 488Hz(default), timer2, 8-bit
 * D11  pwm 488Hz(default), timer2, 8-bit, SPI MOSI hardware
*/
 
//PWM frequency pin D5 or pin D6:  1024 = 61Hz, 256 = 244Hz, 64 = 976Hz(default)
//MotorA (pin D5 or pin D6, prescaler 64)  
  setPWMPrescaler(pwm1, 64);
  
//PWM frequency pin D9 or pin D10: 1024 = 30Hz, 256 = 122Hz, 64 = 488Hz(default), 8 = 3906Hz
//MotorB (pin D9 or pin D10, prescaler 8)  
  setPWMPrescaler(pwm3, 8);

//MotorA/steering ---------------------------------------------------------------------------- 

  if (rc_data.steering < 1450) // < 1500us, dead band of poor quality joysticks
  {
    motA_value = map(rc_data.steering, 1450, 1000, 0, 255);
    analogWrite(pwm1, motA_value); 
    digitalWrite(pwm2, LOW);
  }
  else if (rc_data.steering > 1550) // > 1500us, dead band of poor quality joysticks
  {
    motA_value = map(rc_data.steering, 1550, 2000, 0, 255);
    analogWrite(pwm2, motA_value); 
    digitalWrite(pwm1, LOW);
  }
  else
  {
    digitalWrite(pwm1, HIGH); //"HIGH" brake, "LOW" no brake
    digitalWrite(pwm2, HIGH); //"HIGH" brake, "LOW" no brake
//    analogWrite(pwm1, motA_value = 255); //adjustable brake (0-255)
//    analogWrite(pwm2, motA_value = 255); //adjustable brake (0-255)
  }

//  Serial.println(rc_data.steering); //print value ​​on a serial monitor
  
//MotorB/throttle ----------------------------------------------------------------------------

  if (rc_data.throttle < 1450) // < 1500us, dead band of poor quality joysticks
  {
    motB_value = map(rc_data.throttle, 1450, 1000, 0, 255); 
    analogWrite(pwm3, motB_value); 
    digitalWrite(pwm4, LOW);
  }
  else if (rc_data.throttle > 1550) // > 1500us, dead band of poor quality joysticks
  {
    motB_value = map(rc_data.throttle, 1550, 2000, 0, 255); 
    analogWrite(pwm4, motB_value); 
    digitalWrite(pwm3, LOW);
  }
  else
  {
    digitalWrite(pwm3, LOW); //"HIGH" brake, "LOW" no brake
    digitalWrite(pwm4, LOW); //"HIGH" brake, "LOW" no brake
//    analogWrite(pwm3, motB_value = 127); //adjustable brake (0-255)
//    analogWrite(pwm4, motB_value = 127); //adjustable brake (0-255)
  }
}

//************************************************************************************************************************************************************************
//initial main settings **************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void setup()
{
//  Serial.begin(9600); //print value ​​on a serial monitor
//  printf_begin();     //print the radio debug info

  pinMode(pwm1, OUTPUT);
  pinMode(pwm2, OUTPUT);
  pinMode(pwm3, OUTPUT);
  pinMode(pwm4, OUTPUT);
  
  pinMode(led, OUTPUT);
  pinMode(inRXbatt, INPUT);
  
  resetData();
  attachServoPins();

  //define the radio communication
  radio.begin();
  radio.setAutoAck(true);          //ensure autoACK is enabled (default true)
  radio.enableAckPayload();        //enable custom ack payloads on the acknowledge packets
  radio.enableDynamicPayloads();   //enable dynamically-sized payloads
  radio.setRetries(5, 5);          //set the number and delay of retries on failed submit (max. 15 x 250us delay (blocking !), max. 15 retries)
  
  radio.setChannel(76);            //which RF channel to communicate on (0-125, 2.4Ghz + default 76 = 2.476Ghz) 
  radio.setDataRate(RF24_250KBPS); //RF24_250KBPS (fails for units without +), RF24_1MBPS, RF24_2MBPS
  radio.setPALevel(RF24_PA_MIN);   //RF24_PA_MIN (-18dBm), RF24_PA_LOW (-12dBm), RF24_PA_HIGH (-6dbm), RF24_PA_MAX (0dBm) 

  radio.openWritingPipe(addresses[0]);    //open a pipe for writing via byte array
  radio.openReadingPipe(1, addresses[1]); //open all the required reading pipes, and then call "startListening"
                                          
  radio.startListening(); //set the module as receiver. Start listening on the pipes opened for reading
}

//************************************************************************************************************************************************************************
//program loop ***********************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void loop()
{
  receive_time();
  send_and_receive_data();

  outputServo();
  outputPWM();
  
  battery_voltage();

//  Serial.println("Radio details *****************");
//  radio.printDetails(); //print the radio debug info
 
} //end program loop

//************************************************************************************************************************************************************************
//get time after losing RF data or turning off the TX, reset data and the LED flashing ***********************************************************************************
//************************************************************************************************************************************************************************
unsigned long lastRxTime = 0;

void receive_time()
{
  if(millis() >= lastRxTime + 1000) //1000 (1second)
  {
    resetData();       
    RFoff_check(); 
  }
}

//************************************************************************************************************************************************************************
//send and receive data **************************************************************************************************************************************************
//************************************************************************************************************************************************************************
void send_and_receive_data()
{
  byte pipeNo;
  
  if (radio.available(&pipeNo))
  {
    radio.writeAckPayload(pipeNo, &payload, sizeof(ackPayload));
   
    radio.read(&rc_data, sizeof(packet));
    
    lastRxTime = millis(); //at this moment we have received the data
    RX_batt_check();                     
  } 
}

//************************************************************************************************************************************************************************
//measuring the input of the RX battery. After receiving RF data, the monitored RX battery is activated ******************************************************************
//RX battery voltage 1S LiPo (4.2V) < 3.3V = LEDs RX, TX flash at a interval of 500ms. Battery OK = LEDs RX, TX is lit ***************************************************
//************************************************************************************************************************************************************************
void battery_voltage()
{ 
  payload.RXbatt = analogRead(inRXbatt) * (RX_battery_voltage / 1023);

//  Serial.println(payload.RXbatt); //print value ​​on a serial monitor   
}

//-------------------------------------------------------------------
unsigned long ledTime = 0;
int ledState, detect;

void RX_batt_check()
{ 
  detect = payload.RXbatt <= RX_monitored_voltage;
  
  if (millis() >= ledTime + 500) //1000 (1second)
  {
    ledTime = millis();
    
    if (ledState >= !detect + HIGH)
    {
      ledState = LOW;
    }
    else
    {
      ledState = HIGH;
    }   
    digitalWrite(led, ledState);
  }
}

//************************************************************************************************************************************************************************
//when RX is switched on and TX is switched off, or after the loss of RF data = LED RX flash at a interval of 100 ms. Normal mode = LED RX is lit ************************
//************************************************************************************************************************************************************************
void RFoff_check()
{
  if (millis() >= ledTime + 100) //1000 (1second)
  {
    ledTime = millis();
    
    if (ledState)
    {
      ledState = LOW;
    }
    else
    {
      ledState = HIGH;
    }   
    digitalWrite(led, ledState);
  }
}
   
