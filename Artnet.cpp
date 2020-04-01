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
   //   Serial.println("RR");
      //Udp.read(artnetPacket, MAX_BUFFER_ARTNET);

      // Check that packetID is "Art-Net" else ignore
     /* for (byte i = 0 ; i < 9 ; i++)
      {
        if (artnetPacket[i] != ART_NET_ID[i])
          return 0;
      }*/
      
      opcode = Udp.udpBuffer[8] | (Udp.udpBuffer[9] << 8);//artnetPacket[8] | artnetPacket[9] << 8;
     // Serial.printf("opc ode:%d\n",opcode);

      if (opcode == ART_DMX)
      {
         
        //sequence = artnetPacket[12];
        incomingUniverse = Udp.udpBuffer[14] | (Udp.udpBuffer[15] << 8);//artnetPacket[14] | (artnetPacket[15] << 8);
        dmxDataLength = Udp.udpBuffer[17] | Udp.udpBuffer[16] << 8; //artnetPacket[17] | artnetPacket[16] << 8;
          //Serial.printf("receiving universe n:%d\n",incomingUniverse);
         //Serial.printf("%s\n",artnetPacket+ ART_DMX_START);
          if(nbPixelsPerUniverse*(incomingUniverse)*3<=nbPixels*3)
          {
             if(dmxDataLength>nbPixelsPerUniverse*3)
                {
                 dmxDataLength= nbPixelsPerUniverse*3;
                }
              timef=millis();
              //+currentframenumber*nbPixels*3
               memcpy(&artnetleds1[nbPixelsPerUniverse*(incomingUniverse)*3],Udp.udpBuffer + ART_DMX_START,dmxDataLength);
             if(incomingUniverse==0)
             {
                 // Serial.println("new frame");
                 if((sync |sync2))
                      lostframes++;
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
          //Serial.println(sync)
          
      
       // if (artDmxCallback) (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, remoteIP);
        //return ART_DMX;
      }
     /* if (opcode == ART_POLL)
      {
        //fill the reply struct, and then send it to the network's broadcast address
        Serial.print("POLL from ");
        Serial.print(remoteIP);
        Serial.print(" broadcast addr: ");
        Serial.println(broadcast);

        #if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
          IPAddress local_ip = Ethernet.localIP();
        #else
          IPAddress local_ip = WiFi.localIP();
        #endif
        node_ip_address[0] = local_ip[0];
      	node_ip_address[1] = local_ip[1];
      	node_ip_address[2] = local_ip[2];
      	node_ip_address[3] = local_ip[3];

        sprintf((char *)id, "Art-Net\0");
        memcpy(ArtPollReply.id, id, sizeof(ArtPollReply.id));
        memcpy(ArtPollReply.ip, node_ip_address, sizeof(ArtPollReply.ip));

        ArtPollReply.opCode = ART_POLL_REPLY;
        ArtPollReply.port =  ART_NET_PORT;

        memset(ArtPollReply.goodinput,  0x08, 4);
        memset(ArtPollReply.goodoutput,  0x80, 4);
        memset(ArtPollReply.porttypes,  0xc0, 4);

        uint8_t shortname [18];
        uint8_t longname [64];
        sprintf((char *)shortname, "artnet arduino\0");
        sprintf((char *)longname, "Art-Net -> Arduino Bridge\0");
        memcpy(ArtPollReply.shortname, shortname, sizeof(shortname));
        memcpy(ArtPollReply.longname, longname, sizeof(longname));

        ArtPollReply.etsaman[0] = 0;
        ArtPollReply.etsaman[1] = 0;
        ArtPollReply.verH       = 1;
        ArtPollReply.ver        = 0;
        ArtPollReply.subH       = 0;
        ArtPollReply.sub        = 0;
        ArtPollReply.oemH       = 0;
        ArtPollReply.oem        = 0xFF;
        ArtPollReply.ubea       = 0;
        ArtPollReply.status     = 0xd2;
        ArtPollReply.swvideo    = 0;
        ArtPollReply.swmacro    = 0;
        ArtPollReply.swremote   = 0;
        ArtPollReply.style      = 0;

        ArtPollReply.numbportsH = 0;
        ArtPollReply.numbports  = 4;
        ArtPollReply.status2    = 0x08;

        ArtPollReply.bindip[0] = node_ip_address[0];
        ArtPollReply.bindip[1] = node_ip_address[1];
        ArtPollReply.bindip[2] = node_ip_address[2];
        ArtPollReply.bindip[3] = node_ip_address[3];

        uint8_t swin[4]  = {0x01,0x02,0x03,0x04};
        uint8_t swout[4] = {0x01,0x02,0x03,0x04};
        for(uint8_t i = 0; i < 4; i++)
        {
            ArtPollReply.swout[i] = swout[i];
            ArtPollReply.swin[i] = swin[i];
        }
        sprintf((char *)ArtPollReply.nodereport, "%i DMX output universes active.\0", ArtPollReply.numbports);
        Udp.beginPacket(broadcast, ART_NET_PORT);//send the packet to the broadcast address
        Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
        Udp.endPacket();

        return ART_POLL;
      }
      if (opcode == ART_SYNC)
      {
        if (artSyncCallback) (*artSyncCallback)(remoteIP);
        return ART_SYNC;
      }*/
  }
  else
  {
    //return 0;
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
