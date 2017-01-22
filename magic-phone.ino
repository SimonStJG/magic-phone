#include "SIM900.h"
#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

#include "sms.h"
#include "call.h"

/**
 * Error codes
 */
#define ERROR_UNABLE_TO_INITALIZE 2
#define ERROR_UNABLE_TO_DELETE_SMS 3
#define ERROR_UNABLE_TO_GET_SMS_LIST 4
#define ERROR_UNABLE_TO_SEND_SMS 5
#define ERROR_UNABLE_TO_GET_CALL_STATE 6
#define ERROR_UNABLE_TO_INITIALIZE_AUDIO 7

/**
 * Pins
 */
#define AUDIO_TX 2
#define AUDIO_RX 3
#define AUDIO_RST 4
#define AUDIO_ACT 5

// GSM Shield Serial is 7 and 8.  GSM Shield power pin is 6 (usually it's 9).
#define LED_ERROR 9
#define LED_STATUS 10

/**
 * Baud rates.
 */
#define AUDIO_BAUD_RATE 9600
#define GSM_BAUD_RATE 4800
#define DEBUG_SERIAL_BAUD_RATE 19200

/**
 * Notes:
 * Moved Moved GSM on / off pins in SIM900.h
 */

CallGSM call;

SMSGSM sms;

SoftwareSerial audioSerial = SoftwareSerial(AUDIO_TX, AUDIO_RX);
Adafruit_Soundboard audio = Adafruit_Soundboard(
  &audioSerial, NULL, AUDIO_RST);

// TODO Disable voicemail!
// TODO Mention that DTMF tones need to be held down for a long time..

void setup()
{
  // Switch on both LEDs during startup
  pinMode(LED_ERROR, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_ERROR, HIGH);
  digitalWrite(LED_STATUS, HIGH);

  // Initialise Audio board.
  pinMode(AUDIO_ACT, INPUT);
  Serial.begin(DEBUG_SERIAL_BAUD_RATE);
  audioSerial.begin(AUDIO_BAUD_RATE);  
  if (!audio.reset())
  {
    fail(ERROR_UNABLE_TO_INITIALIZE_AUDIO);
  }

  // Initialize GSM shield.
  if (gsm.begin(GSM_BAUD_RATE)) 
  {
    call.SetDTMF(1);
  } 
  else 
  {
    fail(ERROR_UNABLE_TO_INITALIZE);
  }

  digitalWrite(LED_ERROR, LOW);
  digitalWrite(LED_STATUS, LOW);
}

void loop()
{
  handleIncomingSms();
  handleIncomingCalls();
  pulseStatusLed();
}

void handleIncomingCalls()
{
  char number[20];
  byte stat = 0;
  stat = call.CallStatusWithAuth(number, 0, 0);
  delay(2000);
  switch (stat) 
  {
    case CALL_INCOM_VOICE_AUTH:
    case CALL_INCOM_VOICE_NOT_AUTH:
      digitalWrite(LED_STATUS, HIGH);
      call.PickUp();
      callActive();
      call.HangUp();
      digitalWrite(LED_STATUS, LOW);
      break;
    case CALL_ACTIVE_VOICE:
    case CALL_INCOM_DATA_AUTH:
    case CALL_INCOM_DATA_NOT_AUTH:
    case CALL_ACTIVE_DATA:
      // We don't expect to end up on this path.
      call.HangUp();
      break;
    case CALL_NONE:
      break; 
    case CALL_NO_RESPONSE:
    case CALL_COMM_LINE_BUSY:
      fail(ERROR_UNABLE_TO_GET_CALL_STATE);
      break;
  }
}

/**
 * TODO What if someone hangs up?
 */
void callActive() 
{
//  char dtmf;
//  char all_dtmf[5] = "";
//  
//  while(true)
//  {
//    dtmf = call.DetDTMF();
//    if (dtmf != '-')
//    {
//      Serial.print("Found one");
//      Serial.write(&dtmf, 1);
//      strncat(all_dtmf, &dtmf, 1);
//      if (strlen(all_dtmf) >= 4) 
//      {
//        break;
//      }
//      dtmf = '-';
//    }
//  }
  // TODO Can I get rid of this?
  char number[20];
  int audioStatus;
  byte stat;
  
  audio.playTrack((uint8_t) 0);
  while(true) 
  {
    audioStatus = digitalRead(AUDIO_ACT);
    stat = call.CallStatusWithAuth(number, 0, 0);
    if (audioStatus == HIGH || stat != CALL_ACTIVE_VOICE) break;
    delay(500);
  }
  audio.stop();
}

void handleIncomingSms()
{
  char position;
  char phoneNumber[20];
  char smsText[50];
  
  while (true)
  {
    position = sms.IsSMSPresent(SMS_ALL);
    if (position == 0) break;
    digitalWrite(LED_ERROR, HIGH);
    
    if (sms.GetSMS(position, phoneNumber, 20, smsText, 50) < 0)
    {
      fail(ERROR_UNABLE_TO_GET_SMS_LIST);
    }
    
    // Somehow, giffgaff send you messages all the f***ing time, whose originator 
    // address is just "giffgaff.", and others where the originator is blank.  
    // Then everything explodes if you try and reply.  So we ignore these.
    if (strcmp("", phoneNumber) != 0 &&
        strncmp("giffgaff", phoneNumber, 8) != 0 &&
        sms.SendSMS(phoneNumber, "This number only accepts phone calls.") <= 0) 
    {
      fail(ERROR_UNABLE_TO_SEND_SMS);
    }
     
    if (!sms.DeleteSMS(position))
    {
      fail(ERROR_UNABLE_TO_DELETE_SMS);
    }
    digitalWrite(LED_ERROR, LOW);
  }
  
}

void pulseStatusLed()
{
  digitalWrite(LED_STATUS, HIGH);
  delay(100);
  digitalWrite(LED_STATUS, LOW);
  delay(900);
}

void fail(int code) 
{
  while (true) 
  {
    for (int i = 0; i < code; i++) 
    {
      digitalWrite(LED_ERROR, HIGH);
      delay(300);
      digitalWrite(LED_ERROR, LOW);
      delay(300);
    }
    delay(2000);
  }
}

