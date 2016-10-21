
#include "src/GSM/SIMCOM.h"

//If you want to use the Arduino functions to manage SMS, uncomment the lines below.
#include "src/GSM/sms.h"
SMSGSM sms;

#include "src/MAX7219.h"

/*////////////////////////////////////////////////////////////////////////////////*/

void MAX7219_setup();
void MAX7219_loop();
void MAX7219_Update(char* pn, char* message);

/*////////////////////////////////////////////////////////////////////////////////*/
//Simple sketch to send and receive SMS.

int numdata;
boolean started=false;
char smsbuffer[160] = "Arduino SMS";
char n[20] = "09291234567";	//Replace with your cell number.

const int RX_pin = 2;
const int TX_pin = 3;
const int GSM_ON_pin = A5;

void setup()
{
	//Serial connection.
	Serial.begin(115200);
	Serial.println(F("GSM Shield testing."));

  MAX7219_setup();
  
	//Configure Comm Port to select Hardware or Software serial
#if defined(__AVR_ATmega328P__)
	gsm.SelectSoftwareSerial(RX_pin, TX_pin, GSM_ON_pin);
#else
	gsm.SelectHardwareSerial(&Serial1, GSM_ON_pin);
#endif

	//Configure baudrate.
	if (gsm.begin(9600)) 
	{
		Serial.println(F("\nstatus=READY"));
		started=true;
	} else Serial.println(F("\nstatus=IDLE"));

	if(started) 
	{
		//Enable this two lines if you want to send an SMS.
		if (sms.SendSMS(n,smsbuffer))
			Serial.println(F("\nSMS sent OK"));
	}
};

void loop()
{
	if(started) 
	{
		int pos=sms.IsSMSPresent(SMS_ALL);
		if(pos>0&&pos<=20)
		{
			Serial.print(pos);
			Serial.print(". ");
			int ret_val = sms.GetSMS(pos, n, 20, smsbuffer, 160);
			if (ret_val>0)
			{
				switch (ret_val) {
				case GETSMS_UNREAD_SMS:
					Serial.print(F("UNREAD SMS from "));
					break;
				case GETSMS_READ_SMS:
					Serial.print(F("READ SMS from "));
					break;
				default:  
					Serial.print(F("OTHER SMS from "));
					break;
				}
				Serial.println(n);
				Serial.println(smsbuffer);
				if ((ret_val == GETSMS_UNREAD_SMS) || (ret_val == GETSMS_READ_SMS))
				{
					if (sms.SendSMS(n,smsbuffer))
					{
						Serial.println(F("SMS resent OK"));
					}  
					MAX7219_Update(n, smsbuffer);
				}
				sms.DeleteSMS(pos);
			}
		}
	}
	delay(1000);
};

/*////////////////////////////////////////////////////////////////////////////////*/

static boolean _entered_yield = false;

void (*LED_scroll)(void) = MAX7219_loop;

/*
This is the yeild() function that is called from delay() to utilize
what is otherwise wasted CPU time during the delay.
*/
void yield(void)
{
	if (_entered_yield) return;
	_entered_yield = true;
	LED_scroll();
	_entered_yield = false;
}

/*////////////////////////////////////////////////////////////////////////////////*/

const int ledPin = 13;      // the number of the LED pin
int ledState = LOW;         // ledState used to set the LED

unsigned long previousMillis = 0;
const long interval = 50;

void MAX7219_setup()
{
	LED_Control.Init();
	LED_Control.LoadMessage(F(" LED Matrix SMS display "));
	pinMode(ledPin, OUTPUT);
}

void MAX7219_loop()
{
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= interval)
	{
		// save the last time you blinked the LED
		previousMillis = currentMillis;

		// if the LED is off turn it on and vice-versa:
		if (ledState == LOW) {
			ledState = HIGH;
		}
		else {
			ledState = LOW;
		}

		// set the LED with the ledState of the variable:
		digitalWrite(ledPin, ledState);

		LED_Control.LoadDisplayBuffer();
	}
}

// Update the LED Matrix to display the SMS.
void MAX7219_Update(char* pn, char* message)
{
	String SMSstr(F(" PN:"));
	SMSstr += pn;
	SMSstr += F(" SMS:");
	SMSstr += message;
	LED_Control.LoadMessage(SMSstr.c_str());
}


