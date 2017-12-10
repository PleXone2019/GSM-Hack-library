/***************************************************
  This is an example for our Hackabels GSM Hack Module

  Designed specifically to work with the Adafruit FONA
  ----> https://www.hackables.cc/gsm/12-gsm-hack.html

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface.

  Written by Limor Fried/Ladyada maintained by Luis Gonçalves for GSM Hack.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

/*
THIS CODE IS STILL IN PROGRESS!

Open up the serial console on the Arduino at 38400 baud to interact with FONA


This code will receive an SMS, identify the sender's phone number, and will change the relay state
then it will send and SMS back with the state of the relay.

*/

#include "Adafruit_FONA.h"

#define FONA_RX 4
#define FONA_TX 3
#define FONA_RST 7
#define FONA_POWER 5
#define RELAY 8

char relay_state;

// this is a large buffer for replies
char replybuffer[255];

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup() 
{
  while (!Serial);

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  relay_state = 0;

  pinMode(FONA_POWER, OUTPUT);
  digitalWrite(FONA_POWER, LOW);
  delay(100);
  digitalWrite(FONA_POWER, HIGH);
  delay(1500);
  digitalWrite(FONA_POWER, LOW);

  Serial.begin(38400);
  Serial.println(F("FONA SMS caller ID test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // make it slow so its easy to read!
  fonaSerial->begin(9600);
  if (! fona.begin(*fonaSerial)) 
  {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));

  // Print SIM card IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) 
  {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received

  Serial.println("FONA Ready");
}

  
char fonaInBuffer[64];          //for notifications from the FONA

void loop() 
{
  
  char* bufPtr = fonaInBuffer;    //handy buffer pointer
  
  if (fona.available())      //any data available from the FONA?
  {
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  
    {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaInBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;
    
    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaInBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) 
    {
      Serial.print("slot: "); Serial.println(slot);
      
      char callerIDbuffer[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) 
      {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);
      
      if(relay_state)
      {
        relay_state = 0;
        digitalWrite(RELAY, LOW);
      }
      else
      {
        relay_state = 1;
        digitalWrite(RELAY, HIGH);
      }
      
      //Send back an automatic response
      Serial.println("Sending reponse...");
      if (relay_state? !fona.sendSMS(callerIDbuffer, "Relay ON") : !fona.sendSMS(callerIDbuffer, "Relay OFF") ) 
      {
        Serial.println(F("Failed"));
      } 
      else 
      {
        Serial.println(F("Sent!"));
      }
      
      // delete the original msg after it is processed
      //   otherwise, we will fill up all the slots
      //   and then we won't be able to receive SMS anymore
      if (fona.deleteSMS(slot)) 
      {
        Serial.println(F("OK!"));
      } 
      else 
      {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        fona.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}
