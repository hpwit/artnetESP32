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
#include <lwip/sockets.h>
#include <lwip/netdb.h>

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
void Artnet::getframe(uint8_t* leds)
{
    memcpy(leds,&artnetleds1[((currentframenumber+1)%2)*nbPixels*3],nbPixels*3);
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
void Artnet::begin(uint16_t nbpixels,uint16_t nbpixelsperuniverses)
{
    if(running)
    {
        Serial.println("Artnet Already Running");
        return;
    }
    nbframeread=0;
    currentframenumber=0;
    buffernum=2;
    readbuffer=2-1;
    
    
    
    
    running=true;
    nbPixels=nbpixels;
    nbPixelsPerUniverse=nbpixelsperuniverses;
    nbNeededUniverses=nbPixels/nbPixelsPerUniverse;
    if(nbNeededUniverses*nbPixelsPerUniverse-nbPixels<0) {
        
        nbNeededUniverses++;
    }
    //artnetleds1= (uint8_t *)malloc(nbpixels*buffernumber*3);
    artnetleds1= (uint8_t *)malloc( (nbpixelsperuniverses*3+ART_DMX_START)*nbNeededUniverses*2 +8);
    if(artnetleds1==NULL)
    {
        Serial.printf("impossible to create the buffer\n");
        return;
        
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
    last_size=(nbpixels%nbpixelsperuniverses)*3;
    if(last_size==0)
        last_size=nbpixelsperuniverses*3;
    //create the task for the display
    xTaskCreatePinnedToCore(Artnet::afterFrameTask, "afterFrameTask", 3000, this,1, &artnetAfterFrameHandle, 0);
    xTaskCreatePinnedToCore(Artnet::readFromSDTask, "readFromSDTask", 3000, this,1, &readFromSDHandle, 0);
    
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


/*
 uint16_t Artnet::artnet_task(void *pvParameters)
 {
 for(;;)
 {
 
 }
 }
 */


//void Artnet::getFramedRecord()
//{
//    uint32_t size=nbPixelsPerUniverse*3;
//    uint32_t decal=nbPixelsPerUniverse*3+ART_DMX_START;
//    uint32_t decal2=nbNeededUniverses*decal;
//    // uint8_t * udpof=Udp.udpBuffer + ART_DMX_START;
//    uint8_t * offset;
//    uint8_t * leds=artnetleds1+(nbframeread%2)*decal2;
//    memmove(leds,&elaspe[nbframeread%2],4);
//    leds+=4;
//    offset=artnetleds1+(nbframeread%2)*decal2+ART_DMX_START;
//
//    if(nbNeededUniverses>1)
//    {
//        for(int uni=0;uni<nbNeededUniverses-1;uni++)
//        {
//            memmove(leds,offset,size);
//            offset+=decal;
//            leds+=size;
//        }
//    }
//    memmove(leds,offset,last_size);
//    if(recordfile)
//        recordfile.write(leds,nbPixels*3+4);
//    if(ledsbuffer)
//        memcpy(ledsbuffer,leds+4,nbPixels*3);
//    nbframeread++;
//}


bool Artnet::readNextFrameAndWait(File playingfile)
{
    if(playingfile.available()>0)
    {
        uint32_t waitnewtframe=0;
        playingfile.read(getframeread(buffer_read),nbPixels*3+4);
        waitnewtframe=*((uint32_t*)getframeread(buffer_read));
        totalrecording+=waitnewtframe/240;
        uint32_t time_now=ESP.getCycleCount();
        uint32_t diff=(time_now-timenow);
        if(waitnewtframe>diff)
         delayMicroseconds((waitnewtframe-diff)/240);
        
        timenow=ESP.getCycleCount();
        buffer_play=buffer_read;
        xTaskNotifyGive(readFromSDHandle);
        buffer_read++;
        return true;
    }
    else
    {
        
        Serial.printf("Reading done nbframes:%d totlrecord %ld ms\n",buffer_read,totalrecording/1000);
        buffer_read=0;
        buffer_play=0;
        totalrecording=0;
        return false;
    }
    
}

void Artnet::getFrameForRecord(uint8_t* leds)
{
    uint32_t size=nbPixelsPerUniverse*3;
    uint32_t decal=nbPixelsPerUniverse*3+ART_DMX_START;
    uint32_t decal2=nbNeededUniverses*decal;
    uint8_t * offset;
    recordfile.write((uint8_t*)(&elaspe[nbframeread%2]),4);
    offset=artnetleds1+(nbframeread%2)*decal2+ART_DMX_START;
    if(nbNeededUniverses>1)
    {
        for(int uni=0;uni<nbNeededUniverses-1;uni++)
        {
            memcpy(leds,offset,size);
            offset+=decal;
            leds+=size;
        }
    }
    memcpy(leds,offset,last_size);
    //For an unknown reason if you put the write here, only couple of leds are actually copied ...
    //recordfile.write(leds,nbPixels*3);
    
}



uint8_t * Artnet::getFrameReadSD()
{
    return getframeread(buffer_play)+4;
}

uint8_t * Artnet::getframeread(int buffer)
{
    return artnetleds1+(buffer%2)*(nbPixelsPerUniverse*3*nbNeededUniverses+4);
}
                      
                      
                      
void Artnet::getBufferFrame(uint8_t* leds)
{
    uint32_t size=nbPixelsPerUniverse*3;
    uint32_t decal=nbPixelsPerUniverse*3+ART_DMX_START;
    uint32_t decal2=nbNeededUniverses*decal;
    // uint8_t * udpof=Udp.udpBuffer + ART_DMX_START;
    uint8_t * offset;
    offset=artnetleds1+((currentframenumber+1)%2)*decal2+ART_DMX_START;
    
    if(nbNeededUniverses>1)
    {
        for(int uni=0;uni<nbNeededUniverses-1;uni++)
        {
            memcpy(leds,offset,size);
            offset+=decal;
            leds+=size;
        }
    }
    memcpy(leds,offset,last_size);
    nbframeread++;
}

uint32_t Artnet::getElaspseTime()
{
    return elaspe[(currentframenumber+1)%2];
    
    
}


uint16_t Artnet::read2()
{
    struct sockaddr_in si_other;
    int slen = sizeof(si_other) , len;
    long timef=0;
    
    //timef=millis();
    incomingUniverse=99;
    uint32_t decal=nbPixelsPerUniverse*3+ART_DMX_START;
    uint32_t decal2=nbNeededUniverses*decal;
    uint8_t * offset;
    bool resetframe=true;
    
er:
    timef=millis();
    offset=artnetleds1+currentframenumber*decal2;
    
    while(incomingUniverse!=0)
    {
        if(millis()-timef>1000)
        {
            Serial.println("Time out fired");
            return 0;
        }
        if ((len = recvfrom(Udp.udp_server, offset, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) >0 )//1460
        {
            
            incomingUniverse = offset[14] ;
        }
    }
    if(resetframe==true or frameslues==0)
    {
        current_time=ESP.getCycleCount();
        if(frameslues==0)
        {
            start_time=current_time;
        }
        elaspe[currentframenumber]=(current_time-start_time);
        /*if(elaspe[currentframenumber]/240000<30)
         Serial.printf("frame:%d time:%lu\n",frameslues,elaspe[currentframenumber]/240);*/
        start_time=current_time;
    }
    for(int uni=1;uni<nbNeededUniverses;uni++)
    {
        offset+=decal;
        while((len = recvfrom(Udp.udp_server, offset, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) <=0)
        {
            if(millis()-timef>1000)
            {
                Serial.println("Time out fired");
                return 0;
            }
        }
        incomingUniverse = *(offset+14);
        if(incomingUniverse!=uni)
        {
            lostframes++;
            resetframe=false;
            goto er;
        }
    }
    
    
    
    
    Udp.flush();
    
    
    
    
    currentframenumber=(currentframenumber+1)%2;
    
    frameslues++;

    return 1;
}

uint16_t Artnet::readFrame()
{
    uint16_t result;
    result=read2();
    if(result==1 and artnetAfterFrameHandle)
        xTaskNotifyGive(artnetAfterFrameHandle);
    return result;
}

uint16_t Artnet::readFrameRecord()
{
    uint16_t result;
    result=read2();
    return result;
}


uint16_t Artnet::read2(TaskHandle_t task)
{
    struct sockaddr_in si_other;
    int slen = sizeof(si_other) , len;
    long timef=0;
    sync=0;
    sync2=0;
    timef=millis();
    incomingUniverse=99;
    uint32_t size=nbPixelsPerUniverse*3;
    uint32_t decal=nbPixelsPerUniverse*3+ART_DMX_START;
    uint32_t decal2=nbNeededUniverses*decal;
    uint8_t * udpof=Udp.udpBuffer + ART_DMX_START;
    uint8_t *offset2=artnetleds1;
    offset2+=currentframenumber*decal2;
    uint8_t * offset;
    timef=millis();
    for(;;){
        
    er:
        
        offset=offset2;//+currentframenumber*nbPixels*3;
        
        //er:
        
        while(incomingUniverse!=0)
        {
            if(millis()-timef>1000)
            {
                Serial.println("Time out fired");
                return 0;
            }
            if ((len = recvfrom(Udp.udp_server, offset, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) >0 )//1460
            {
                
                incomingUniverse = offset[14] ;//+ Udp.udpBuffer[15];
                
                
            }
            
        }
        
        for(int uni=1;uni<nbNeededUniverses;uni++)
        {
            offset+=decal;
            while((len = recvfrom(Udp.udp_server, offset, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) <=0)
            {
                if(millis()-timef>1000)
                {
                    Serial.println("Time out fired");
                    return 0;
                }
            }
            incomingUniverse = *(offset+14);
            if(incomingUniverse!=uni)
            {
                lostframes++;
                goto er;
                
            }
        }
        
        
        frameslues++;
        Udp.flush();
        currentframenumber=(currentframenumber+1)%2;
        xTaskNotifyGive(task);
        //currentframenumber=(currentframenumber+1)%2;
        //memcpy(&artnetleds1[nbPixelsPerUniverse*(incomingUniverse)*3+currentframenumber*nbPixels*3],Udp.udpBuffer + ART_DMX_START,nbPixelsPerUniverse*3);
    }
    
    
    return 1;
}

void Artnet::setLedsBuffer(uint8_t * leds)
{
    ledsbuffer=leds;
}

void Artnet::startArtnetrecord(File file)
{
    if(stopRecord)
    {
        recordfile=file;
        stopRecord=false;
        firstframe=false;
        xTaskCreatePinnedToCore(Artnet::recordArtnetTask, "recordArtnetTask", 3000, this,1, &recordArtnetHandle, 0);
    }
}

void Artnet::stopArtnetRecord()
{
    stopRecord=true;
}

void Artnet::recordArtnetTask(void *pvParameters)
{
    Artnet * artnet= (Artnet*)pvParameters;
    artnet->nbframeread=0;
    artnet->frameslues=0;
    artnet->lostframes=0;
    Serial.println("Ready to record");
    while(!artnet->stopRecord)
    {
        if(artnet->nbframeread<artnet->frameslues)
        {
            if(artnet->frameslues==1)
            {
                artnet->recordStartTime=millis();
                Serial.println("First Frame recorded\n");
            }
            artnet->getFrameForRecord(artnet->ledsbuffer);
           artnet->recordfile.write(artnet->ledsbuffer,artnet->nbPixels*3);
             artnet->nbframeread++;
            if(artnet->frameRecordCallback)
                (*artnet->frameRecordCallback)();
        }
        TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
        TIMERG0.wdt_feed=1;                       // feed dog
        TIMERG0.wdt_wprotect=0;
    }
    artnet->recordfile.close();
    artnet->recordEndTime=millis();
    Serial.printf("Record stopped framesartnet:%d framesrecorded:%d  lostsframes:%d duration:%ld ms\n",artnet->frameslues,artnet->frameslues,artnet->lostframes,artnet->recordEndTime-artnet->recordStartTime);
    
    artnet->nbframeread=0;
    artnet->frameslues=0;
    artnet->lostframes=0;
    vTaskDelete(NULL);
}

void Artnet::afterFrameTask(void *pvParameters)
{
    Artnet * artnet= (Artnet*)pvParameters;
    for(;;) {
        
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        if(artnet->ledsbuffer!=NULL)
            artnet->getBufferFrame(artnet->ledsbuffer);
        if (((Artnet*)pvParameters)->frameCallback)
            (*((Artnet*)pvParameters)->frameCallback)();
        
    }
}

void Artnet::readFromSDTask(void *pvParameters)
{
  Artnet * artnet= (Artnet*)pvParameters;
  for(;;) {
      
      ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
      if(artnet->ledsbuffer!=NULL)
          memcpy(artnet->ledsbuffer,artnet->getframeread(artnet->buffer_read)+4,artnet->nbPixels*3);
      if (((Artnet*)pvParameters)->readFromSDCallback)
          (*((Artnet*)pvParameters)->readFromSDCallback)();
      
  }
}


uint16_t Artnet::read()
{
    
    struct sockaddr_in si_other;
    int slen = sizeof(si_other) , len;
    long timef=0;
    //for(;;){
    sync=0;
    sync2=0;
    timef=millis();
    while(sync!=syncmax or sync2!=syncmax2 )
    {
        if(millis()-timef>1000)
        {
            Serial.println("Time out fired");
            return 0;
        }
        
        //packetSize = Udp.parsePacket2();
        
        /*char * buf = new char[1460];
         if(!buf){
         return 0;
         }*/
        if ((len = recvfrom(Udp.udp_server, Udp.udpBuffer, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) >0 )//1460
            
        {
            incomingUniverse = Udp.udpBuffer[14];// | (Udp.udpBuffer[15] << 8);//artnetPacket[14] | (artnetPacket[15] << 8);
            timef=millis();
            
            memcpy(&artnetleds1[nbPixelsPerUniverse*(incomingUniverse)*3],Udp.udpBuffer + ART_DMX_START,nbPixelsPerUniverse*3);
            
            if(incomingUniverse==0)
            {
                //Serial.println("*************new frame**************");
                if(sync |sync2){
                    
                    
                    lostframes++;
                    
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
    
    frameslues++;
    Udp.flush();
    //currentframenumber=(currentframenumber+1)%2;
    //  xTaskNotifyGive(task);
    //}
    return 1;
}


uint16_t Artnet::read(TaskHandle_t task)
{
    
    struct sockaddr_in si_other;
    int slen = sizeof(si_other) , len;
    long timef=0;
    for(;;){
        sync=0;
        sync2=0;
        timef=millis();
        while(sync!=syncmax or sync2!=syncmax2 )
        {
            if(millis()-timef>1000)
            {
                Serial.println("Time out fired");
                return 0;
            }
            
            //packetSize = Udp.parsePacket2();
            
            /*char * buf = new char[1460];
             if(!buf){
             return 0;
             }*/
            if ((len = recvfrom(Udp.udp_server, Udp.udpBuffer, 800, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) >0 )//1460
                
            {
                incomingUniverse = Udp.udpBuffer[14] ;//| (Udp.udpBuffer[15] << 8);//artnetPacket[14] | (artnetPacket[15] << 8);
                timef=millis();
                memcpy(&artnetleds1[nbPixelsPerUniverse*(incomingUniverse)*3+currentframenumber*nbPixels*3],Udp.udpBuffer + ART_DMX_START,nbPixelsPerUniverse*3);
                
                if(incomingUniverse==0)
                {
                    //Serial.println("*************new frame**************");
                    if(sync |sync2){
                        
                        
                        lostframes++;
                        
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
        
        frameslues++;
        Udp.flush();
        //currentframenumber=(currentframenumber+1)%2;
        xTaskNotifyGive(task);
    }
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
