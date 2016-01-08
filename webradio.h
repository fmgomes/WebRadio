/*
 2015 Copyright (c) Ideavity Lda.

 Authors: Fernando Gomes

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#ifndef webradio_h
#define webradio_h

#if defined(ARDUINO) && ARDUINO >= 100
#define SEEEDUINO
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <pgmspace.h>
#include "SPI.h"
#include "ESP8266WiFi.h"
#include "WiFiClient.h"


#define SCI_MODE 0x0
#define SCI_STATUS 0x1
#define SCI_BASS 0x2
#define SCI_CLOCKF 0x3
#define SCI_DECODE_TIME 0x4
#define SCI_AUDATA 0x5
#define SCI_WRAM 0x6
#define SCI_WRAMADDR 0x7
#define SCI_HDAT0 0x8
#define SCI_HDAT1 0x9
#define SCI_AIADDR 0xa
#define SCI_VOL 0xb
#define SCI_AICTRL0 0xc
#define SCI_AICTRL1 0xd
#define SCI_AICTRL2 0xe
#define SCI_AICTRL3 0xf
#define SCI_num_registers 0xf

#define SM_DIFF 0
#define SM_LAYER12 1
#define SM_RESET 2
#define SM_OUTOFWAV 3
#define SM_EARSPEAKER_LO 4
#define SM_TESTS 5
#define SM_STREAM 6
#define SM_EARSPEAKER_HI 7
#define SM_DACT 8
#define SM_SDIORD 9
#define SM_SDISHARE 10
#define SM_SDINEW 11
#define SM_ADPCM 12
#define SM_ADCPM_HP 13
#define SM_LINE_IN 14
#define SM_CLK_RANGE 15

#define VS1053_CHUNK_SIZE 32

#define RBSIZE 32768
#define RBHIGH 13107
#define RBLOW  0

#define min(a,b) (((a)<(b))?(a):(b))




#define FULL_BUFFER		-1000
#define EMPTY_BUFFER	-1000

typedef struct ringbuffer {
	int read_pos;
	int write_pos;
	uint32_t size;
	uint8_t *buffer;
} ringbuffer;

class Ring
{
private:
	ringbuffer rbuf;

public:
	int Init(int size);
	void Clear(void);
	void Destroy();
	int Write(uint8_t* data, int count);
	int Read(uint8_t* data, int count);
	unsigned int Avail();
	unsigned int Free();
	
};



class WebRadio
{

private:

	uint8_t _CS;
	uint8_t _DCS;
	uint8_t _DREQ;
	uint8_t _RST;
	uint16_t ADTSBlockSize;
	uint16_t ADTSMissingBytes;
	WiFiClient client;
	uint8_t buf[4096];
	bool playing;
	unsigned int noplay;
	unsigned int noplaycnt;
	Ring rb;
	char lastHost[64];
	char lastUrl[64];
	uint16_t lastPort;
	uint16_t lastVol;
	
	void DataModeOn(void);
	void DataModeOff(void);
	void SdiSendBuffer(const uint8_t* data, size_t len);
	void WaitForDREQ(void);
	void WriteRegister(uint8_t _reg, uint16_t _value);
	void ControlModeOn(void);
	void ControlModeOff(void);
// 	void PrintDetails(void);
	uint16_t ReadRegister(uint8_t _reg);
	void ReInit(void);
	

public:

	void Init (char CS, char DCS, char DREQ, char RST);
	bool WiFiConnect(char* ssid, char* password);
	bool Connect(char* myHost, char* myUrl, uint16_t myPort);
	int Loop(char* metaName, char* metaURL);
	void SetVolume(uint16_t vol);
	void PrintDetails(void);
	void PrintDebug(void);
	bool Reconnect(void);
};

#endif
