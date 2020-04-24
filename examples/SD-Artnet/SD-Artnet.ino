#include "WiFi.h"
#include "SD.h"
#include "SPI.h"

#include <Artnet.h>
#include "FastLED.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
FASTLED_USING_NAMESPACE
#define FASTLED_SHOW_CORE 0

#define NB_STRIPS  4 
#define NB_LEDS_PER_STRIP 462 
#define NUM_LEDS NB_STRIPS * NB_LEDS_PER_STRIP
#define UNIVERSE_SIZE 170
CRGB leds[NUM_LEDS];

static uint8_t frame[NUM_LEDS*3];
uint8_t readbuffer[NUM_LEDS*3];
Artnet artnet;


#define JINX_CORE 0
#define SD_CORE 0

static  TaskHandle_t FastLEDshowTaskHandle2 = 0;
static  TaskHandle_t userTaskHandle = 0;
static TaskHandle_t checkWifiTaskHandle;
static TaskHandle_t userTaskHandle2 = 0;
static uint32_t deb=0;
static uint32_t fin=0;;

volatile bool debounceToggle=false;
const byte PIN1 = 2; //23;//2 ;
const byte PIN2 = 25  ;
const byte PIN3 = 33  ;
const byte PIN4 = 32 ;

const byte PIN_NEXT = 27;//27;//5;//27  ;
const byte PIN_PREV = 26  ;
const byte PIN_RECORD =  4;//12 ;//19;//12  ;
const byte PIN_TOGGLE_JINX_SD =13;// 21;//13 ;

volatile bool record_status=false;
volatile bool isJinx=false; //this will command the fact tha we do start by jinx.

char READ_NAME[]="savedata";
File root;
File  myFile;


char SAVE_NAME[]="savedata";

volatile bool new_record=false;
volatile int recordingnumber=0;
volatile bool stopped=false;
volatile bool nextfile=false;
volatile bool prevfile=false;
volatile bool switch_disp_mode=true;
volatile bool switch_record_mode=false;
volatile bool sd_card_present=false;
#define FILE_SIZE 100
#define MAX_FILE 50
char filenames[MAX_FILE * FILE_SIZE];
char filename[FILE_SIZE];
volatile int currentfile=0;

void checkWifi()
{
    if (userTaskHandle2 == 0) {
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        userTaskHandle2 = xTaskGetCurrentTaskHandle();
        xTaskNotifyGive(checkWifiTaskHandle);
        //to thge contrary to the other one we do not wait for the display task to come back
    }
}



void checkWifiTask(void *pvParameters)
{
  
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 500 );
    // -- Run forever...
    for(;;) {
         vTaskDelay(1 );

        while( (!new_record) && record_status && isJinx)
        {
            vTaskDelay(1 );
            if(artnet.nbframeread+1<artnet.frameslues)        //Buffersize was 10
            {
                fin=millis();
                memcpy(leds,artnet.getframe(),NUM_LEDS*sizeof(CRGB));
                myFile.write((uint8_t *)leds,NUM_LEDS*sizeof(CRGB));
                deb=deb+millis()-fin;
                FastLED.show();
            }
            
        }
        
    }
    
}


void selectPrevFile()
{
   Serial.printf("Opening Prev File");
     if(recordingnumber==0)
     {
      Serial.println("no file\n");
      return;
     }
   Serial.printf("current file :%d total%d\n", currentfile,recordingnumber);
  if(currentfile==0)
    currentfile=recordingnumber-1;
   else
   currentfile--;
   
    char fi[FILE_SIZE];
    memset(fi,0,FILE_SIZE);
    sprintf(fi,"%s",&filenames[currentfile*FILE_SIZE]);
    myFile=SD.open(fi);
    Serial.printf("Opening file %s\n",myFile.name());
   
}


void selectNextFile()
{
    if(!sd_card_present)
    {
        Serial.println("No card mounted");
        return;
    }
    Serial.println("looking for next file");
    if(recordingnumber==0)
     {
      Serial.println("no file\n");
      return;
     }
     Serial.printf("current file number:%d\n",currentfile);
    currentfile=(currentfile+1)%recordingnumber;
    Serial.printf("new file number:%d\n",currentfile);
    char fi[FILE_SIZE];
    memset(fi,0,FILE_SIZE);
    sprintf(fi,"%s",&filenames[currentfile*FILE_SIZE]);
    Serial.printf("try to open file at potision %d/%d %s %s\n",currentfile,recordingnumber,fi, &filenames[currentfile*FILE_SIZE]);
    myFile=SD.open(fi);
    Serial.printf("Opening file %s\n",myFile.name());
   
}


void openNextFileForRecording()
{
  if(myFile)
      myFile.close();
      
  int recordnum=0;
  if(recordingnumber>0)
     recordnum=recordingnumber-1;
    else
    recordnum=recordingnumber;
    
    if(!sd_card_present)
    {
        Serial.println("No card mounted");
        return;
    }
    memset(filename, 0, FILE_SIZE);
    sprintf(filename,"/%s_%03d.dat",SAVE_NAME,recordnum);
    myFile = SD.open(filename); //, FILE_WRITE);
    while(myFile)
    {
        myFile.close();
        recordnum++;
        memset(filename, 0, FILE_SIZE);
        sprintf(filename,"/%s_%03d.dat",SAVE_NAME,recordnum);
        myFile = SD.open(filename); //, FILE_WRITE);
         
    }
    recordingnumber++;
    
    myFile = SD.open(filename, FILE_WRITE);
    if(!myFile)
    {
      
      Serial.printf("Unable to open %s for writing\n",filename);
    }
    else
    {
      sprintf(&filenames[(recordingnumber-1)*FILE_SIZE],"%s",myFile.name());
      Serial.printf("Opening file :%s for recording position %d\n",myFile.name(),recordingnumber-1);
    }
}


void numberfiles()
{
  memset(filenames,0,MAX_FILE * FILE_SIZE);
    recordingnumber=0;
    root=SD.open("/");
    myFile = root.openNextFile();
    while(myFile)
    {
        if(!myFile.isDirectory())
        {
          
           sprintf(&filenames[recordingnumber*FILE_SIZE],"%s",myFile.name());
          
            myFile.close();
            recordingnumber++;
            
        }
        myFile = root.openNextFile();
    }
    //recordingnumber++;
    if(recordingnumber>0)
    {
          char fi[FILE_SIZE];
          memset(fi,0,FILE_SIZE);
          sprintf(fi,"%s",&filenames[0]);
          myFile=SD.open(fi);
          Serial.printf("Opening file %s\n",myFile.name());
          currentfile=0;
          root.rewindDirectory();
          Serial.printf("Number of files already stored:%d\n",recordingnumber);
    }
    else
    {
      Serial.println("No file stored");
    }
    
}



void FastLEDshowESP322()
{
    if (userTaskHandle == 0) {
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        userTaskHandle = xTaskGetCurrentTaskHandle();
        xTaskNotifyGive(FastLEDshowTaskHandle2);
    }
}




void FastLEDshowTask2(void *pvParameters)
{
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 500 );
    // -- Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        memcpy(leds,artnet.getframe(),NUM_LEDS*sizeof(CRGB));
        FastLED.show();
        userTaskHandle=0; //so we can't have two display tasks at the same time
        
    }
}

void IRAM_ATTR ISR_Next() {
    if(debounceToggle || nextfile)
        return;
    debounceToggle=true;
    nextfile=true;
    debounceToggle=false;
}

void IRAM_ATTR ISR_Prev() {
      if(debounceToggle || prevfile)
        return;
    debounceToggle=true;
    prevfile=true;
    debounceToggle=false;
}


void IRAM_ATTR ISR_Toggle() {
    if(debounceToggle || switch_disp_mode)
        return;
    debounceToggle=true;
   // isJinx= digitalRead(PIN_TOGGLE_JINX_SD)==0 ? true:false;
   isJinx=!isJinx;
    switch_disp_mode=true;
    //delay(1000);
    debounceToggle=false;
}

/* to uncomment if the code above is not working
 
void IRAM_ATTR ISR_Toggle() {
    if(debounceToggle)
        return;
    debounceToggle=true;
    isJinx= digitalRead(PIN_TOGGLE_JINX_SD)==0 ? true:false;
    switch_disp_mode=true;
    delay(1000);
    debounceToggle=false;
}
 */

void setup() {
    Serial.begin(115200);
    
    //create the Interrupt services for the button
    pinMode(PIN_NEXT, INPUT_PULLDOWN);
     pinMode(PIN_PREV, INPUT_PULLDOWN);
    pinMode(PIN_RECORD, INPUT_PULLDOWN);
    pinMode(PIN_TOGGLE_JINX_SD,INPUT_PULLDOWN);

    attachInterrupt(PIN_NEXT, ISR_Next, RISING);
    attachInterrupt(PIN_PREV, ISR_Prev, RISING);
    attachInterrupt(PIN_TOGGLE_JINX_SD, ISR_Toggle, CHANGE);
    
    xTaskCreatePinnedToCore(FastLEDshowTask2, "FastLEDshowTask2", 3000, NULL,1, &FastLEDshowTaskHandle2, FASTLED_SHOW_CORE);
    
    
    xTaskCreatePinnedToCore(checkWifiTask, "checkWifiTask", 3000, NULL,3, &checkWifiTaskHandle, FASTLED_SHOW_CORE);

    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        sd_card_present=false;
    }
    else
    {
        uint8_t cardType = SD.cardType();
        if(cardType == CARD_NONE){
            Serial.println("No SD card attached");
            sd_card_present=false;
        }
        else
        {
            sd_card_present=true;
            //root=SD.open("/");
            numberfiles();
            //selectNextFile();
        }
    }

    isJinx= digitalRead(PIN_TOGGLE_JINX_SD)==0 ? true:false;
    record_status= digitalRead(PIN_RECORD)==1 ? true:false;
    
    
    WiFi.mode(WIFI_STA);
    
    Serial.printf("Connecting ");
    WiFi.begin("", "");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(WiFi.status());
   
      
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.softAPIP());
    delay(4000);
   
    
    // put your setup code here, to run once:
    FastLED.addLeds<WS2812,  PIN1,  GRB>(leds, 0, NB_LEDS_PER_STRIP);
    FastLED.addLeds<WS2812,  PIN2, GRB>(leds, NB_LEDS_PER_STRIP, NB_LEDS_PER_STRIP);
    FastLED.addLeds<WS2812,  PIN3, GRB>(leds, NB_LEDS_PER_STRIP*2, NB_LEDS_PER_STRIP);
    FastLED.addLeds<WS2812,  PIN4, GRB>(leds, NB_LEDS_PER_STRIP*3, NB_LEDS_PER_STRIP);
    FastLED.show();
    artnet.begin(NUM_LEDS,UNIVERSE_SIZE,1);
    checkWifi();
}

void loop() {
    if(!isJinx )
    {
        if(switch_disp_mode)
        {
            Serial.println("SD-Card");
            switch_disp_mode=false;
            
        }
        if(!myFile)
        {
            Serial.println("no file open ... looking for net file");
            //root=SD.open("/");
            selectNextFile();
            
        }
        
        if (myFile.available()>0)
        {
            myFile.read(readbuffer,NUM_LEDS*sizeof(CRGB));//NUM_LEDS*sizeof(CRGB));
            memcpy(leds,readbuffer,NUM_LEDS*sizeof(CRGB));
            FastLED.show();
            delay(20);
        }
        
        else
        {
            myFile.seek(0);
        }
        
        if(nextfile && !isJinx)
        {
            selectNextFile();
            delay(500);
            nextfile=false;
        }
        if(prevfile && !isJinx)
        {
            selectPrevFile();
            delay(500);
            prevfile=false;
        }
    }
    else
    {
        
        if( switch_disp_mode)
        {
            Serial.println("From Jinx");
            artnet.resetsync();
            switch_disp_mode=false;
        }
        record_status=digitalRead(PIN_RECORD);
        
        if(!record_status)
        {
            artnet.read();
            //memcpy(leds,artnet.getframe(),NUM_LEDS*sizeof(CRGB));
            //FastLED.show();
            FastLEDshowESP322();
            artnet.resetsync();
        }
        else
        {
            if(record_status && switch_record_mode==false)
            {
                new_record=true;
                stopped=false;
                switch_record_mode=true;
            }
            
            while(record_status){
                if(new_record)
                {
                    switch_record_mode=false;
                    artnet.nbframeread=0;
                    artnet.frameslues=0;
                    artnet.lostframes=0;
                    if(myFile)
                        myFile.close();
                    openNextFileForRecording(); //this will avoid to override existing files
                    new_record=false;
                    Serial.println("start new recording");
                }
                
                if(!new_record)
                {
                    artnet.read();
                    if(artnet.nbframeread==0) //because we already did a getframe();
                    {
                        Serial.println("Recording has started");
                        Serial.printf("Filename:%s\n",filename);
                    }
                }
                record_status=digitalRead(PIN_RECORD);
            }
            if(!new_record)
            {
                myFile.close();
                Serial.println("Recording stopped ...");
                Serial.printf("frames saved:%d frames lues:%d lost frames:%d\n",artnet.nbframeread,artnet.frameslues,artnet.lostframes);
                Serial.printf("time per frame %f\n",(float)deb/(artnet.nbframeread));
                artnet.nbframeread=0;
                artnet.frameslues=0;
                artnet.lostframes=0;
                deb=0;
                stopped=true;
                new_record=true;
                //recordingnumber++;
                switch_record_mode=false;
            }
        }
    }
}
