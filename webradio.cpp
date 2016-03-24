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
 Foundation, Inc.,51 Franklin St,Fifth Floor, Boston, MA 02110-1301 USA

*/

#include <Arduino.h>
#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "SPI.h"
#include "FS.h"
#include "webradio.h"

void WebRadio::Init (char CS, char DCS, char DREQ, char RST)
{
	_CS = CS;
	_DCS = DCS;
	_DREQ = DREQ;
	_RST = RST;
	playing = false;
		
    pinMode(12, FUNCTION_2); //MISO
    pinMode(13, FUNCTION_2); //MOSI
    pinMode(14, FUNCTION_2); //SCLK     
    SPI.begin();   // Start SPI
	if(_RST!=0xFF) {
		pinMode(_RST,OUTPUT);  
		digitalWrite(_RST,LOW);
	}
	// The SCI and SDI will start deselected
//*	pinMode(_RST,OUTPUT);     
//*	digitalWrite(_RST,HIGH);
	pinMode(_CS,OUTPUT);    
	digitalWrite(_CS,HIGH);
	if(_DCS!=0xFF) {
		pinMode(_DCS,OUTPUT);    
		digitalWrite(_DCS,HIGH);		
	}
	pinMode(_DREQ,INPUT);   // DREQ is an input
	
    Serial.println("Booting VS1053...");   // Boot VS1053D
	delay(10);
	
	SPI.setClockDivider(SPI_CLOCK_DIV64); // Slow!
	if(_RST!=0xFF) {
		digitalWrite(_RST,HIGH);    //Mp3ReleaseFromReset();
	}
	delay(10);
	// Declick: Immediately switch analog off
//	WriteRegister(SCI_VOL,0xffff); // VOL
	/* Declick: Slow sample rate for slow analog part startup */
//	WriteRegister(SCI_AUDATA,10);
	delay(100);
	/* Switch on the analog parts */
//	WriteRegister(SCI_VOL,0xfefe); // VOL
//	WriteRegister(SCI_AUDATA,44101); // 44.1kHz stereo
	SetVolume(80);
	// soft reset
	if(_DCS!=0xFF)
		WriteRegister(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
	else
		WriteRegister(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET) | _BV(SM_SDISHARE));
	delay(1);
	WaitForDREQ();
//	WriteRegister(SCI_CLOCKF,0xB800); // Experimenting with higher clock settings
	WriteRegister(SCI_CLOCKF,0x6000); // Experimenting with higher clock settings
	delay(1);
	WaitForDREQ();
	SPI.setClockDivider(SPI_CLOCK_DIV4); // Now you can set high speed SPI clock - Fastest available   
	ApplyPatch(plugin, PLUGIN_SIZE);
	
	PrintDetails();
	
/*	From now on made dinamically on the connect methode if not done previously
	if (rb.Init(RBSIZE)==-1)
		Serial.printf("No memory for rb");
*/

}

void WebRadio::ReInit(void) 
{

    Serial.println("Rebooting VS1053...");  	
	SPI.setClockDivider(SPI_CLOCK_DIV64); // Slow!
	if(_RST!=0xFF) {
		digitalWrite(_RST,LOW);
		delay(10);
		digitalWrite(_RST,HIGH);    //Mp3ReleaseFromReset();
	}
	delay(10);
	// Declick: Immediately switch analog off
	WriteRegister(SCI_VOL,0xffff); // VOL
	/* Declick: Slow sample rate for slow analog part startup */
	WriteRegister(SCI_AUDATA,10);
	delay(100);
	/* Switch on the analog parts */
	WriteRegister(SCI_VOL,0xfefe); // VOL
	WriteRegister(SCI_AUDATA,44101); // 44.1kHz stereo
	SetVolume(lastVol);
//	WriteRegister(SCI_VOL,); // VOL
	// soft reset
	if(_DCS!=0xFF)
		WriteRegister(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
	else
		WriteRegister(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET) | _BV(SM_SDISHARE));
	delay(1);
	WaitForDREQ();
	WriteRegister(SCI_CLOCKF,0xB800); // Experimenting with higher clock settings
	delay(1);
	WaitForDREQ();
	SPI.setClockDivider(SPI_CLOCK_DIV4); // Now you can set high speed SPI clock - Fastest available   
	ApplyPatch(plugin, PLUGIN_SIZE);

	PrintDetails();
}

bool WebRadio::WiFiConnect(char* ssid, char* password)
{
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);    
		Serial.print(".");
	}
	Serial.println("");  
	Serial.println("WiFi connected");  
	Serial.println("IP address: ");  
	Serial.println(WiFi.localIP());
}


bool WebRadio::Connect(char* myHost, char* myUrl, uint16_t myPort)
{
	char* pbuf;
	char* pbuf2;
	uint16_t n;
	char tmp[256];
	
	// Test if the ringbufer was already created
	if(rb.rbuf.buffer == 0) {
		if (rb.Init(RBSIZE)==-1)
			Serial.printf("No memory for rb");		
	}
	
	fileSize = -1;
	filePos = 0;
				
	if(client.connected())
		client.stop();
	
	strcpy(lastHost, myHost);
	strcpy(lastUrl, myUrl);
	lastPort = myPort;
	
	if (!client.connect(myHost, myPort)) {
		Serial.println("connection failed");
		return false;
	}
  
// OK with metadata:
//	client.print(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Icy-MetaData: 1\r\nConnection: close\r\n\r\n");
// OK without metadata:
//	client.print(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Connection: close\r\n\r\n");

	Serial.println(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Icy-MetaData: 1\r\nRange: bytes=0-\r\nUser-Agent: VLC/2.2.1 LibVLC/2.2.1\r\nConnection: close\r\n\r\n");
	client.print(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Icy-MetaData: 1\r\nRange: bytes=0-\r\nUser-Agent: VLC/2.2.1 LibVLC/2.2.1\r\nConnection: close\r\n\r\n");

	Serial.println(client.status());
	Serial.println(client.available());
  
	if(client.connected()){    
		unsigned long next = millis() + 5000;
		while(client.available()<2048) {
			yield(); 
			if ((long)(millis()-next) > 0) {
				Serial.println(">5s waiting for server reply");
				Serial.println(client.available());
				client.read(buf, client.available());
				Serial.println((char*)buf);
				return false;
			}
		}
		client.read(buf, 2048);
		Serial.println((char*)buf);
		
		Serial.println("Station Information:");
		pbuf = strstr((char*)buf, "icy-name:");
		if (pbuf) {
			pbuf2 = strstr((char*)pbuf, "\r\n");
			strncpy(tmp, pbuf+9, pbuf2-pbuf-9);
			tmp[pbuf2-pbuf-9] = 0;
			Serial.println(tmp);
		}
		pbuf = strstr((char*)buf, "icy-genre:");
		if (pbuf) {
			pbuf2 = strstr((char*)pbuf, "\r\n");
			strncpy(tmp, pbuf+10, pbuf2-pbuf-10);
			tmp[pbuf2-pbuf-10] = 0;
			Serial.println(tmp);
		}
		pbuf = strstr((char*)buf, "icy-url:");
		if (pbuf) {
			pbuf2 = strstr((char*)pbuf, "\r\n");
			strncpy(tmp, pbuf+8, pbuf2-pbuf-8);
			tmp[pbuf2-pbuf-8] = 0;
			Serial.println(tmp);
		}

		pbuf = strstr((char*)buf, "icy-metaint:");
		if(pbuf) {
			Serial.println("Has metadata");
			ADTSBlockSize = atoi(pbuf+12);
			Serial.println(ADTSBlockSize);
			pbuf = strstr(pbuf+12, "\r\n\r\n") + 4;
			n = 2048-(pbuf-(char*)buf);
			ADTSMissingBytes = ADTSBlockSize - n;
			Serial.println(ADTSMissingBytes);
			metaData = true;
		} else {
			Serial.println("No metadata");
			metaData = false;
			pbuf = strstr((char*)buf, "Content-Length:");
			if(pbuf) {
				fileSize = atol(pbuf+16);
				filePos = 0;
//				Serial.println(pbuf);
			} 
			pbuf = strstr((char*)buf, "ID3");
			if(pbuf) {
				Serial.println("ID3v2");
				uint32_t mp3Offset;
				mp3Offset = ((uint32_t)(*(pbuf+9))); 
				mp3Offset += ((uint32_t)(*(pbuf+8)))<<7;
				mp3Offset += ((uint32_t)(*(pbuf+7)))<<14;
				mp3Offset += ((uint32_t)(*(pbuf+6)))<<21;
				mp3Offset += 10;
				Serial.println(mp3Offset);
				client.stop();
				if (!client.connect(myHost, myPort)) {
					Serial.println("connection failed");
					return false;
				}
				client.print(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Icy-MetaData: 1\r\nRange: bytes=" + mp3Offset + "-\r\nUser-Agent: VLC/2.2.1 LibVLC/2.2.1\r\nConnection: close\r\n\r\n");
				Serial.println(String("GET ") + myUrl + " HTTP/1.1\r\n" + "Host: " + myHost + ":" + myPort + "\r\n" + "Icy-MetaData: 1\r\nRange: bytes=" + mp3Offset + "-\r\nUser-Agent: VLC/2.2.1 LibVLC/2.2.1\r\nConnection: close\r\n\r\n");
				if(client.connected()){    
					unsigned long next = millis() + 5000;
					while(client.available()<2048) {
						yield(); 
						if ((long)(millis()-next) > 0) {
							Serial.println(">5s waiting for server reply");
							Serial.println(client.available());
							client.read(buf, client.available());
							Serial.println((char*)buf);
							return false;
						}
					}
				}
				filePos = mp3Offset;
			}
		}
//		SdiSendBuffer((uint8_t*)pbuf, n);
		playing = false;
		noplay = millis();
		noplaycnt = 0;
		rb.Clear();
		return true;
	}
	return false;
}

bool WebRadio::Reconnect(void)
{
	if (strlen(lastHost))
		return Connect(lastHost, lastUrl, lastPort);
	else
		return false;
}

unsigned long next;

int WebRadio::Loop(char* metaName, char* metaURL)
{
	char* pbuf;
	char* pbuf2;
	uint16_t n;
	unsigned long now;
	
	if(client.connected()) {
		
		n = client.available();
				
		if(metaData && (n>ADTSMissingBytes))
			n = ADTSMissingBytes;
		if (n > 4096) 
			n = 4096;

		n = min(n, rb.Free());
		if (n > 0) {
//			Serial.println(n);
			client.read(buf, n);
			filePos += n;
			if(metaData)
				ADTSMissingBytes = ADTSMissingBytes - n;  
			if (rb.Write(buf, n) != n)
				Serial.println("Error RB Write");
		}			
		// Fazer play a partir do buffer circular - ok
		// Aumentar o buffer (não da mais de 32k e não parece ter vantagem sequer em ter os 32k), meter o playing a meio do buffer (start a 80%, stop a 0%, já está) - ok
		// Quando deixa de tocar por falta de dados, ao fim de pouco tempo deveria quebrar a ligação e forçar religar ao mesmo - ok
		// Verificar se faz sentido gastar cerca de 100ms a enviar 2048 bytes para o VS1053. Os mesmos 2048 bytes vão mais depressa se o bitrrate for 192 em vez de 128kbps
		// Perceber a razão do vs1053 se 'desprogramar' de vez em quando - volume no max, etc. - e como resolver / contornar isso - A alteração não resolveu completamente porque colocou o valor errado do volume, ver porquê
		// Na leitura do socket pode passar a ler o minimo dos bytes disponiveis / espaço disponivel no buffer (linhas 154 e seguintes) - ok
		// A estação 2 faz crashar várias vezes com Exception 29, procesamento dos metadados? Colocar protecções nos rretornos do strcmp - ok
		// Compatibilizar entre icecast (ok), shoutcast, (outras?), por causa dos metadados - ok
		// Testar com outras estações icecast, testar com estação shoutcast - ok
		// Estações AAC parecem ter problemas com o VS1053, desprogramando-o, p.e. radio.Connect("192.152.23.242", "/", 8450); 
		// Nas comutações de estações deveria fazer um fade out até 0, esvaziar o buffer e conectar à nova, repondo o volume
		
		int i = rb.Avail();
//		Serial.print("D: "); Serial.print(i); Serial.print(" "); Serial.println(millis()-last); last = millis();
		if ((i>RBHIGH) && !playing) {
			playing = true;
//			Serial.println("Playing true");
		}
		if ((i<=RBLOW) && playing) {
			playing = false;
			noplay = millis();
			noplaycnt++;
//			Serial.println("Playing false");
		}
		if (playing) {
			if(i>2048)
				i = 2048;
			if (rb.Read(buf, i) != i)
				Serial.println("Error RB Read");
			SdiSendBuffer(buf, i);
//			Serial.println(i);
		} else {
			if ((millis() - noplay) > 2000) {
				Serial.println(">2s without playing");
				client.stop();
				return(-1);
			}
		}
		
		if((long)(millis()-next)>=0) {
			if ((ReadRegister(SCI_STATUS) & 0x00f0) != 0x40) {
				Serial.println("VS1053 error!!!!!!!!");
//				SetVolume(lastVol);
				ReInit();
			}
			next = millis()+1000;
		}
			
		if(metaData && (ADTSMissingBytes == 0)) {			
			ADTSMissingBytes = ADTSBlockSize;
			// Metadata block, wait for the number of 16 byte blocks of metadata
			now = millis();
			while(client.available()<1) {
				yield();
				if (millis() > now+1000) {
					Serial.println(">1s waiting metadata 1");
					return(-1);
				}
			}
			n = client.peek()*16+1;
			// Now wait for the metadata blocks
			while(client.available()<n) {
				yield();
				if (millis() > now+1000) {
					Serial.println(">1s waiting metadata 2");
					return(-1);
				}
			}
			// Read the metadata
			client.read(buf, n);
			// If larger then 1, has metadata, process it
			if(n>1) {
				char* pbuf2;
				pbuf = strstr((char*)buf, "'");
				if (pbuf) {
					pbuf2 = strstr(pbuf, ";"); 
					if (pbuf2) {
						pbuf2--;
						*pbuf2 = 0;
						if (metaName)
							strcpy(metaName, pbuf+1); 
						}
				}
				pbuf = strstr(pbuf2+1, "'");
				if (pbuf) {
					pbuf2 = strstr(pbuf+1, "'");
					if (pbuf2) {
						*pbuf2 = 0;
						if (metaURL)
							strcpy(metaURL, pbuf+1);					
					}
				}
				return 1;
			}
		}
		return 0;
	} else {
		return -1;
	}
}

void WebRadio::SetVolume(uint16_t vol)
{
	
	lastVol = vol;	
	vol = 0xFE * (100-vol) / 100;
	vol |= vol << 8;
	Serial.print("Vol: "); Serial.println(vol, HEX);
	WriteRegister(SCI_VOL, vol);
}

uint16_t WebRadio::GetVolume()
{
	return(lastVol);	
}

bool WebRadio::PlayFile(char* fileName)
{
	uint8_t buf[1024];
	uint16_t n, i;
/*
	SdiSendZeros(2052);
	uint16_t mode = ReadRegister(SCI_MODE);
	WriteRegister(SCI_MODE, mode | _BV(SM_OUTOFWAV));
	SdiSendZeros(2052);
*/	
	File localFile = SPIFFS.open(fileName, "r");
	if (!localFile)
		return false;
	
	n = localFile.size();
	while (n) {
		if (n > 1024) {
			i = 1024;
			n = n-1024;
		} else {
			i = n;
			n = 0;
		}
		localFile.readBytes((char*)buf, i);
		SdiSendBuffer(buf, i);
	}

//	SdiSendZeros(2048);

	return true;
}

void WebRadio::ApplyPatch(const uint16_t *patch, uint16_t patchsize) 
{

	uint16_t i = 0;

	Serial.print("Patch size: "); Serial.println(patchsize);
	while ( i < patchsize ) {
		uint16_t addr, n, val;

//		addr = pgm_read_word(patch++);
		addr = *patch++;
//		n = pgm_read_word(patch++);
		n = *patch++;
		i += 2;
//		Serial.println(addr, HEX);
		if (n & 0x8000U) { // RLE run, replicate n samples 
			n &= 0x7FFF;
//			val = pgm_read_word(patch++);
			val = *patch++;
			i++;
			while (n--) {
				WriteRegister(addr, val);
			}      
		} else {           // Copy run, copy n samples 
			while (n--) {
//				val = pgm_read_word(patch++);
				val = *patch++;
				i++;
				WriteRegister(addr, val);
			}
		}
	}
}

void WebRadio::AdjustRate(long ppm2) 
{	
	WriteRegister(SCI_WRAMADDR, 0x1e07);
	WriteRegister(SCI_WRAM, ppm2);
	WriteRegister(SCI_WRAM, ppm2 >> 16);
	/* oldClock4KHz = 0 forces adjustment calculation when rate checked. */
	WriteRegister(SCI_WRAMADDR, 0x5b1c);
	WriteRegister(SCI_WRAM, 0);
	/* Write to AUDATA or CLOCKF checks rate and recalculates adjustment. */
	WriteRegister(SCI_AUDATA, ReadRegister(SCI_AUDATA));
}


void WebRadio::SetClock(uint16_t clock)
{
	WriteRegister(SCI_CLOCKF,0xB800 | clock);
}

void WebRadio::SdiSendBuffer(const uint8_t* data, size_t len)
{
	DataModeOn();
	while ( len ) {
		//Serial.println("await_data_request");
		WaitForDREQ();
		delayMicroseconds(3);
		size_t chunk_length = min(len, VS1053_CHUNK_SIZE);
		len -= chunk_length;
		while ( chunk_length-- )
			SPI.transfer(*data++);
	}
	DataModeOff();
//  Serial.println("sdi sb end");
}

void WebRadio::SdiSendZeros(size_t len)
{
	DataModeOn();
	while ( len ) {
		//Serial.println("await_data_request");
		WaitForDREQ();
		delayMicroseconds(3);
		size_t chunk_length = min(len, VS1053_CHUNK_SIZE);
		len -= chunk_length;
		while ( chunk_length-- )
			SPI.transfer(0);
	}
	DataModeOff();
}

void WebRadio::DataModeOn(void) 
{
    digitalWrite(_CS, HIGH);
	if(_DCS!=0xFF)
		digitalWrite(_DCS, LOW);
}

void WebRadio::DataModeOff(void)
{
	if(_DCS!=0xFF)
		digitalWrite(_DCS, HIGH);
}  

void WebRadio::WaitForDREQ(void) 
{
    while (!digitalRead(_DREQ)) {
      yield();
//      Serial.print("!");    
    }
}

void WebRadio::ControlModeOn(void) 
{
	if(_DCS!=0xFF)
		digitalWrite(_DCS, HIGH);
    digitalWrite(_CS, LOW);
}
void WebRadio::ControlModeOff(void) 
{
    digitalWrite(_CS, HIGH);
}  

void WebRadio::WriteRegister(uint8_t _reg, uint16_t _value) 
{
  ControlModeOn();
  delayMicroseconds(1); // tXCSS
  SPI.transfer(B10); // Write operation
  SPI.transfer(_reg); // Which register
  SPI.transfer(_value >> 8); // Send hi byte
  SPI.transfer(_value & 0xff); // Send lo byte
  delayMicroseconds(1); // tXCSH
  WaitForDREQ();
  ControlModeOff();
}

uint16_t WebRadio::ReadRegister(uint8_t _reg) 
{
  uint16_t result;
  ControlModeOn();
  delayMicroseconds(1); // tXCSS
  SPI.transfer(0x03); // Read operation
  SPI.transfer(_reg); // Which register
  result = SPI.transfer(0xff) << 8; // read high byte
  result |= SPI.transfer(0xff); // read low byte
  delayMicroseconds(1); // tXCSH
  WaitForDREQ();
  ControlModeOff();
  return result;
}

void WebRadio::ChangeModeMIDItoMP3()
{
	// Esses 4 linhas torna bordo para ser executado no modo de MP3, sem solda necessária mais 
	WriteRegister (SCI_WRAMADDR, 0xc017); 	// Endereço de GPIO_DDR é 0xC017 
	WriteRegister (SCI_WRAM, 0x0003); 		// = 3 GPIO_DDR 
	WriteRegister (SCI_WRAMADDR, 0xc019); 	// Endereço de GPIO_ODATA é 0xC019 
	WriteRegister (SCI_WRAM, 0x0000); 		// 0 = GPIO_ODATA	
}


void WebRadio::PrintDetails(void) 
{
	//spi_saver_t spi_saver;
	Serial.println("VS1053 Configuration:");
	int i = 0;
	while ( i <= SCI_num_registers ) {
		Serial.print(" 0x");
		Serial.print(ReadRegister(i++), HEX);	  
	}
	Serial.print("Vol: "); 
	Serial.println(lastVol);
}

// Ok: 0x800 0x48 0x0 0xB800 0x0 0x1F40 0x0 0x0 0x0 0x0 0x0 0x2020 0x0 0x0 0x0 0x0

/*
128kbps = 16kBps
2KB todos os 125ms
*/


void WebRadio::PrintDebug(void)
{
	Serial.println("Debug:");
	unsigned int avail = rb.Avail();
	
	Serial.print("Buffer used: "); Serial.print(avail); Serial.print(" "); Serial.println(100*avail/RBSIZE);
	Serial.print("Number of failures: "); Serial.println(noplaycnt);
	Serial.print("Playing: "); Serial.println(playing);
	
}

int Ring::Init(int size)
{
	if (size > 0){
        rbuf.size = size;
        if( !(rbuf.buffer = (uint8_t *) malloc(sizeof(uint8_t)*size)) ){
            return -1;
        }
    } else {
        return -1;
    }
    rbuf.read_pos = 0;     
    rbuf.write_pos = 0;
    return 0;
}

void Ring::Clear()
{
    rbuf.read_pos = 0;     
    rbuf.write_pos = 0;
}

void Ring::Destroy()
{
    free(rbuf.buffer);
}

int Ring::Write(uint8_t *data, int count)
{

    int free, pos, rest;

	cli();
	
    if (count <=0 ) return 0;
    pos  = rbuf.write_pos;
    rest = rbuf.size - pos;
    free = Free();

    if ( free < count ){
		sei();
        return FULL_BUFFER;
    }
        
    if (count >= rest){
        memcpy (rbuf.buffer+pos, data, rest);
        if (count - rest)
            memcpy (rbuf.buffer, data+rest, count - rest);
        rbuf.write_pos = count - rest;
    } else {
        memcpy (rbuf.buffer+pos, data, count);
        rbuf.write_pos += count;
    }

	sei();
    return count;
}

int Ring::Read(uint8_t *data, int count)
{

    int avail, pos, rest;

    if (count <=0 ) return 0;
    pos  = rbuf.read_pos;
    rest = rbuf.size - pos;
    avail = Avail();
        
    if ( avail < count ){
        return EMPTY_BUFFER;
    }

    if ( count < rest ){
        memcpy(data, rbuf.buffer+pos, count);
        rbuf.read_pos += count;
    } else {
        memcpy(data, rbuf.buffer+pos, rest);
        if ( count - rest)
            memcpy(data+rest, rbuf.buffer, count - rest);
        rbuf.read_pos = count - rest;
    }

    return count;
}

unsigned int Ring::Free()
{

    int free;
	
    free = rbuf.read_pos - rbuf.write_pos;
    if (free <= 0) free += rbuf.size;
    //Note: free is gauranteed to be >=1 from the above
    return free - 1;
}

unsigned int Ring::Avail()
{

    int avail;
    
	avail = rbuf.write_pos - rbuf.read_pos;
    if (avail < 0) avail += rbuf.size;
        
    return avail;
}
