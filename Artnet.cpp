/*The MIT License (MIT)
 
 Copyright (c) 2014 Nathanaël Lécaudé
 https://github.com/natcl/Artnet, http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <Artnet.h>

Artnet::Artnet() {}

uint32_t Artnet::getsync()
{
    return sync;
}

void Artnet::resetsync()
{
    sync=0;
    sync2=0;
    
}

uint8_t * Artnet::getframe(int framenumber)
{
    /*if(framenumber<2)
     return frames[framenumber];
     else*/
    return NULL;
}

uint8_t * Artnet::getframe()
{
    uint8_t rd=readbuffer;
    readbuffer=(readbuffer+1)%buffernum;
    nbframeread++;
    //        Serial.printf("Read framebuff:%d\n",rd);
    return &artnetleds1[nbPixels*3*rd];
    
}

void Artnet::begin(byte mac[], byte ip[])
{
#if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
    Ethernet.begin(mac,ip);
#endif
    
    Udp.begin(ART_NET_PORT);
}

void Artnet::stop()
{
    if(running)
    {
        Udp.stop();
        //for(int i=0;i<buffernum;i++)
        //  if(frames[i]!=NULL)
        free(artnetleds1);
        running=false;
    }
    
}
void Artnet::begin(uint16_t nbpixels,uint16_t nbpixelsperuniverses,uint8_t buffernumber)
{
    if(running)
    {
        Serial.println("Artnet Already Running");
        return;
    }
    nbframeread=0;
    currentframenumber=0;
    buffernum=buffernumber;
    readbuffer=buffernumber-1;
    
    
    artnetleds1= (uint8_t *)malloc(nbpixels*buffernumber*3);
    if(artnetleds1==NULL)
    {
        Serial.printf("impossible to create the buffer\n");
        return;
        
    }
    running=true;
    nbPixels=nbpixels;
    nbPixelsPerUniverse=nbpixelsperuniverses;
    nbNeededUniverses=nbPixels/nbPixelsPerUniverse;
    if(nbNeededUniverses*nbPixelsPerUniverse-nbPixels<0) {
        
        nbNeededUniverses++;
    }
    Serial.printf("Starting Artnet nbNee sdedUniverses:%d\n",nbNeededUniverses);
    
    if(nbNeededUniverses<=32)
    {
        if(nbNeededUniverses<32)
            syncmax=(1<<nbNeededUniverses)-1;
        else
            syncmax=0xFFFFFFFF;
        syncmax2=0;
    }
    else
    {
        syncmax=0xFFFFFFFF;
        if(nbNeededUniverses-32<32)
            syncmax2=(1<<(nbNeededUniverses-32))-1;
        else
            syncmax2=0xFFFFFFFF;
        //syncmax2=0;
        
    }
    
    Udp.begin(ART_NET_PORT);
}

void Artnet::begin()
{
    Udp.begin(ART_NET_PORT);
}
void Artnet::setBroadcast(byte bc[])
{
    //sets the broadcast address
    broadcast = bc;
}

uint16_t Artnet::read()
{
    return read(false);
}
uint16_t Artnet::read(bool give)
{
    long timef=0;
    sync=0;
    sync2=0;
    //Serial.printf("save framebuff:%d\n",currentframenumber);
    //currentframe=frames[currentframenumber];
    remoteIP = Udp.remoteIP();
    
    timef=millis();
    while(sync!=syncmax or sync2!=syncmax2 )
    {
        if(millis()-timef>1000)
        {
            Serial.println("Time out fired");
            return 0;
        }
        
        packetSize = Udp.parsePacket2();
        //Serial.printf("packetsize:%d\n",packetSize);
        //      Serial.printf("remorteip:%d.%d.%d.%d.%d\n",remoteIP[0],remoteIP[1],remoteIP[2],remoteIP[3],remoteIP[4]);
        // Serial.println("RR");
        if (packetSize <= MAX_BUFFER_ARTNET && packetSize > 0)
        {
            opcode = Udp.udpBuffer[8] | (Udp.udpBuffer[9] << 8);//artnetPacket[8] | artnetPacket[9] << 8;
            // Serial.printf("opc ode:%d\n",opcode);
            
            if (opcode == ART_DMX)
            {
                
                //sequence = artnetPacket[12];
                incomingUniverse = Udp.udpBuffer[14] | (Udp.udpBuffer[15] << 8);//artnetPacket[14] | (artnetPacket[15] << 8);
                dmxDataLength = Udp.udpBuffer[17] | Udp.udpBuffer[16] << 8; //artnetPacket[17] | artnetPacket[16] << 8;
                //Serial.printf("receiving universe n:%d size:%d\n",incomingUniverse,dmxDataLength);
                //Serial.printf("%s\n",artnetPacket+ ART_DMX_START);
                
                
                if(nbPixelsPerUniverse*(incomingUniverse)*3<=nbPixels*3)
                {

                    if(dmxDataLength>nbPixelsPerUniverse*3)
                    {
                        dmxDataLength= nbPixelsPerUniverse*3;
                    }
                    timef=millis();
                     memcpy(&artnetleds1[nbPixelsPerUniverse*(incomingUniverse)*3+currentframenumber*nbPixels*3],Udp.udpBuffer + ART_DMX_START,dmxDataLength);
                    //+currentframenumber*nbPixels*3
                    if(incomingUniverse==0)
                    {
                        //Serial.println("*************new frame**************");
                        if((sync |sync2)){
                           //Serial.println("lost frame");
                            frameslues++;
                            lostframes++;
                            if(readbuffer==0)
                                readbuffer=buffernum-1;
                            else
                                readbuffer-=1;
                            if(give)
                                xSemaphoreGive(Artnet_Semaphore2);
                        }
                        sync=1;
                        sync2=0;
                    }
                    else{
                        if(incomingUniverse<32)
                            sync=sync  | (1<<incomingUniverse);
                        else
                            sync2=sync2  | (1<<(incomingUniverse-32));
                    }
                    
                }
                
            }
            
        }

        // Udp.flush(); //uncomment this if needed
    }
    //Serial.printf("ici");
    /* if((int)(frameslues/buffernum)>(int)(nbframeread/buffernum) and (frameslues%buffernum > nbframeread%buffernum))
     depassment++;*/
    frameslues++;
    currentframenumber=(currentframenumber+1)%buffernum;
    sync=0;
    sync2=0;
    if(give)
        xSemaphoreGive(Artnet_Semaphore2);
    return 1;
}

void Artnet::printPacketHeader()
{
    Serial.print("packet size = ");
    Serial.print(packetSize);
    Serial.print("\topcode = ");
    Serial.print(opcode, HEX);
    Serial.print("\tuniverse number = ");
    Serial.print(incomingUniverse);
    Serial.print("\tdata length = ");
    Serial.print(dmxDataLength);
    Serial.print("\tsequence n0. = ");
    Serial.println(sequence);
}

void Artnet::printPacketContent()
{
    for (uint16_t i = ART_DMX_START ; i < dmxDataLength ; i++){
        Serial.print(artnetPacket[i], DEC);
        Serial.print("  ");
    }
    Serial.println('\n');
}
