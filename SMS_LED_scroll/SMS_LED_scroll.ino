
#include "src/GSM/SIMCOM.h"

//If you want to use the Arduino functions to manage SMS, uncomment the lines below.
#include "src/GSM/sms.h"
SMSGSM sms;

#define Monitor SerialUSB

/*////////////////////////////////////////////////////////////////////////////////*/
//Simple sketch to send and receive SMS.

int numdata;
boolean started=false;
char smsbuffer[160] = "Arduino SMS";
char n[20] = "09297895641";	//Replace with your cell number.

const int RX_pin = 2;
const int TX_pin = 3;
const int GSM_ON_pin = 0;  //Use pin A5 for eGizmo, 7 for Alexan, 0 for others.  

unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 1000;           // interval at which to blink (milliseconds)

void setup()
{
	//Serial connection.
	Monitor.begin(9600);
	Monitor.println(F("GSM Shield testing."));

	//Configure Comm Port to select Hardware or Software serial
#ifdef __arm__
  gsm.SelectHardwareSerial(&Serial1, GSM_ON_pin);
#else
  #if defined(HAVE_HWSERIAL1)
  gsm.SelectHardwareSerial(&Serial1, GSM_ON_pin);
  #else
  gsm.SelectSoftwareSerial(RX_pin, TX_pin, GSM_ON_pin);
  #endif
#endif

	//Configure baudrate.
	if (gsm.begin(9600)) 
	{
		Monitor.println(F("\nstatus=READY"));
		started=true;
	} else Monitor.println(F("\nstatus=IDLE"));

	if(started) 
	{
		//Enable this two lines if you want to send an SMS.
		if (sms.SendSMS(n,smsbuffer))
			Monitor.println(F("\nSMS sent OK"));
	}
};

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) 
  {
    // save the last time you blinked the LED
    //previousMillis = currentMillis - previousMillis;
    //Monitor.println(previousMillis);
    previousMillis = currentMillis;
    
  	if(started) 
  	{
  		int pos=sms.IsSMSPresent(SMS_ALL);
  		if(pos>0&&pos<=20)
  		{
  			Monitor.print(pos);
  			Monitor.print(". ");
  			int ret_val = sms.GetSMS(pos, n, 20, smsbuffer, 160);
  			if (ret_val>0)
  			{
  				switch (ret_val) {
  				case GETSMS_UNREAD_SMS:
  					Monitor.print(F("UNREAD SMS from "));
  					break;
  				case GETSMS_READ_SMS:
  					Monitor.print(F("READ SMS from "));
  					break;
  				default:  
  					Monitor.print(F("OTHER SMS from "));
  					break;
  				}
  				Monitor.println(n);
  				Monitor.println(smsbuffer);
  				if ((ret_val == GETSMS_UNREAD_SMS) || (ret_val == GETSMS_READ_SMS))
  				{
  					if (sms.SendSMS(n,smsbuffer))
  					{
  						Monitor.println(F("SMS resent OK"));
  					}  
  				}
  				sms.DeleteSMS(pos);
  			}
  		}
  	}
  }
};

