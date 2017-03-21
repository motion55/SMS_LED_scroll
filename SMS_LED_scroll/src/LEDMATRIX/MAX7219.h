
#ifndef	_MAX7219_H_
#define	_MAX7219_H_

#include <Arduino.h>

#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#include "SPI.h"
#endif

const int numDevices = 4;      // number of MAX7219s used

#ifdef __AVR_ATmega2560__
const int SPI_MOSI = PIN_SPI_MOSI;
const int SPI_CLK = PIN_SPI_SCK;
const int SPI_CS = PIN_SPI_SS;
#else
const int SPI_MOSI = 4;
const int SPI_CLK = 6;
const int SPI_CS = 5;
#endif

/*////////////////////////////////////////////////////////////////////////////////*/

#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

/*////////////////////////////////////////////////////////////////////////////////*/

class MAX7219Control {
private:
	int LoadPos;
	int ScrollPos;

#define _USE_STRING_CB_

#ifdef _USE_STRING_CB_
	int ColumnBufferLen;
	unsigned char *ColumnBuffer;
#else  
	static const int ColumnBufferLen = 256;
	unsigned char ColumnBuffer[ColumnBufferLen];
#endif

	static const unsigned char font7x5[] PROGMEM;
	static const char font7x5_kern[] PROGMEM;
	static const int font7x5_offset[] PROGMEM;
	
	void spiTransfer(int maxbytes, byte *spidata) 
	{
		//enable the line 
		digitalWrite(_SPI_CS, LOW);
		//Now shift out the data 
		for (int i = maxbytes; i > 0; i--)
		{
#ifdef _SPI_H_INCLUDED
			SPI.transfer(spidata[i - 1]);
#else
			shiftOut(_SPI_MOSI, _SPI_CLK, MSBFIRST, spidata[i - 1]);
#endif
		}
		//latch the data onto the display
		digitalWrite(_SPI_CS, HIGH);
	}

	void spiTransfer(int addr, volatile byte opcode, volatile byte data) 
	{
		//Create an array with the data to shift out
		byte spidata[16];
		/* The array for shifting the data to the devices */
		int offset = addr * 2;
		int maxbytes = maxDevices * 2;

		for (int i = 0; i<maxbytes; i++)
			spidata[i] = (byte)0;
		//put our device data into the array
		spidata[offset + 1] = opcode;
		spidata[offset] = data;

		spiTransfer(maxbytes, spidata);
	}

	/* Data is shifted out of this pin*/
	int _SPI_MOSI;
	/* The clock is signaled on this pin */
	int _SPI_CLK;
	/* This one is driven LOW for chip selection */
	int _SPI_CS;
	/* The maximum number of devices we use */
	int maxDevices;
public:
	MAX7219Control(int dataPin, int clkPin, int csPin, int numDevices) 
	{
		_SPI_MOSI = dataPin;
		_SPI_CLK = clkPin;
		_SPI_CS = csPin;
		if (numDevices <= 0 || numDevices > 8)
			numDevices = 8;
		maxDevices = numDevices;
#ifdef _SPI_H_INCLUDED
		SPI.begin();
#else
		pinMode(_SPI_MOSI, OUTPUT);
		pinMode(_SPI_CLK, OUTPUT);
		pinMode(_SPI_CS, OUTPUT);
		digitalWrite(_SPI_CS, HIGH);
#endif
		//pinMode(PIN_SPI_SS, OUTPUT);
		//digitalWrite(PIN_SPI_SS, HIGH); //To enusre SS pin is HIGH
		for (int i = 0; i < maxDevices; i++) 
		{
			spiTransfer(i, OP_DISPLAYTEST, 0);
			//scanlimit is set to max on startup
			setScanLimit(i, 7);
			//decode is done in source
			spiTransfer(i, OP_DECODEMODE, 0);
			clearDisplay(i);
			//we go into shutdown-mode on startup
			shutdown(i, true);
		}
#ifdef _USE_STRING_CB_
		ColumnBuffer = NULL;
		ColumnBufferLen = 0;
#endif
		LoadPos = 0;
		ScrollPos = 0;
	}

	static MAX7219Control& GetInstance()
	{
		static MAX7219Control* s_instance;
		if (!s_instance) s_instance = new MAX7219Control(SPI_MOSI, SPI_CLK, SPI_CS, numDevices);
		return *s_instance;
	}

	void shutdown(int addr, bool b)
	{
		if (addr<0 || addr >= maxDevices)
			return;
		if (b)
			spiTransfer(addr, OP_SHUTDOWN, 0);
		else
			spiTransfer(addr, OP_SHUTDOWN, 1);
	}

	void setScanLimit(int addr, int limit) 
	{
		if (addr<0 || addr >= maxDevices)
			return;
		if (limit >= 0 && limit<8)
			spiTransfer(addr, OP_SCANLIMIT, limit);
	}

	void setIntensity(int addr, int intensity) 
	{
		if (addr<0 || addr >= maxDevices)
			return;
		if (intensity >= 0 && intensity<16)
			spiTransfer(addr, OP_INTENSITY, intensity);
	}

	void clearDisplay(int addr) 
	{
		if (addr<0 || addr >= maxDevices)
			return;
		for (int i = 0; i<8; i++) {
			spiTransfer(addr, i + 1, 0);
		}
	}

	void setRow(int row, byte *RowData) 
	{
		if (row < 0 || row>7)
			return;
		byte spidata[16];
		/* The array for shifting the data to the devices */

		//put our device data into the array
		for (int addr = 0; addr < maxDevices; addr++)
		{
			int offset = addr * 2;
			spidata[offset + 1] = row + 1;
			spidata[offset] = RowData[addr];
		}

		int maxbytes = maxDevices * 2;
		spiTransfer(maxbytes, spidata);
	}

	void Init()
	{
		for (int x = 0; x<maxDevices; x++)
		{
			shutdown(x, false);       //The MAX72XX is in power-saving mode on startup
			setIntensity(x, 0);       // Set the brightness to minimum value
			clearDisplay(x);         // and clear the display
		}
	}

	char LoadColumnBuffer(char ascii)
	{
		char kern = 0;
		if (ascii >= 0x20 && ascii <= 0x7f)
		{
			kern = pgm_read_byte_near(font7x5_kern + (ascii - 0x20));
			int offset = pgm_read_word_near(font7x5_offset + (ascii - 0x20));

			for (int i = 0; i < kern; i++)
			{
				if (LoadPos >= ColumnBufferLen) return i;
				ColumnBuffer[LoadPos++] = pgm_read_byte_near(font7x5 + offset);
				offset++;
			}
		}
		return kern;
	}

	void ResetColumnBuffer()
	{
		ScrollPos = 0;
#ifdef _USE_STRING_CB_
		if (ColumnBuffer) free(ColumnBuffer);
		ColumnBuffer = NULL;
		ColumnBufferLen = LoadPos = 0;
#else
		for (int col = 0; col < ColumnBufferLen; col++)
		{
			ColumnBuffer[col] = 0;
		}
		LoadPos = 0;
#endif
	}

#ifdef _USE_STRING_CB_
	unsigned char ResizeColumnBuffer(unsigned int BufferLen)
	{
		unsigned char *newBuffer = (unsigned char *)realloc(ColumnBuffer, BufferLen);
		if (newBuffer)
		{
			ColumnBuffer = newBuffer;
			ColumnBufferLen = BufferLen;
			return 1;
		}
		return 0;
	}

	int CheckMessage(const char *message)
	{
		int EndPos = LoadPos;
		for (int counter = 0; ; counter++)
		{
			unsigned char ascii = message[counter];
			if (ascii!=0)
			{
				if (ascii >= 0x20 && ascii <= 0x7f)
				{
					char kern = pgm_read_byte_near(font7x5_kern + (ascii - 0x20));
					EndPos += kern;
				}
			}
			else break;
		}
		return EndPos;
	}
#endif

	int LoadMessage(const char *message)
	{
		ResetColumnBuffer();
#ifdef _USE_STRING_CB_
		int EndPos = CheckMessage(message);
		if (EndPos > ColumnBufferLen)
		{
			if (!ResizeColumnBuffer(EndPos)) return LoadPos;
		}
#endif
		for (int counter = 0; ; counter++)
		{
			unsigned char myChar = message[counter];
			if (myChar != 0)
			{
				LoadColumnBuffer(myChar);
			}
			else break;
		}
		return LoadPos;
	}

	int LoadMessage(const __FlashStringHelper *message)
	{
		String mess(message);
		return LoadMessage(mess.c_str());
	}

	void ResetScrollPos(void)
	{
		ScrollPos = 0;
	}

	int LoadDisplayBuffer()
	{
		int BufferLen = LoadPos;
		if (ScrollPos >= BufferLen) ScrollPos = 0;

		unsigned char mask = 0x01;

		for (int row = 0; row < 8; row++)
		{
			byte RowBuffer[numDevices];
			int Pos = ScrollPos;
			unsigned char dat = 0;

			for (int device = numDevices - 1; device >= 0; device--)
			{
				unsigned char dat = 0;
				for (int col = 0; col < 8; col++)
				{
					dat <<= 1;
					if (Pos >= BufferLen) Pos = 0;
					if (mask & ColumnBuffer[Pos++])
					{
						dat += 1;
					}
					RowBuffer[device] = dat;
				}
			}
			mask <<= 1;
			setRow(row, RowBuffer);
		}
		ScrollPos++;

		return ScrollPos;
	}
};

/*////////////////////////////////////////////////////////////////////////////////*/

#define LED_Control	(MAX7219Control::GetInstance())

#endif	//#ifndef	_MAX7219_H_
