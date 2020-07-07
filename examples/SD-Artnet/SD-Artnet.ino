
#include "SD.h"
#include "SPI.h"
#include <ArtnetESP32.h>

#include "FastLED.h"
FASTLED_USING_NAMESPACE

//The following has to be adapted to your specifications
#define LED_WIDTH 123
#define LED_HEIGHT 16
#define NUM_LEDS LED_WIDTH*LED_HEIGHT
#define UNIVERSE_SIZE 170 //my setup is 170 leds per universe no matter if the last universe is not full.
CRGB leds[NUM_LEDS];
File myFile;

ArtnetESP32 artnet;

bool record_status=false;
bool has_recorded=false;
bool sd_card_present=false;
const byte PIN_RECORD =  14;


void recordfunction()
{
  if (artnet.frameslues%100==0)
   Serial.printf("nb frames read: %d  nb of incomplete frames:%d lost:%.2f %%\n",artnet.frameslues,artnet.lostframes,(float)(artnet.lostframes*100)/artnet.frameslues);
   //here the buffer is the led array hence a simple FastLED.show() is enough to display the array
   FastLED.show(); //if the array is really big I would not put any FastLED.show() because it takes time
}


void setup() {
  Serial.begin(115200);


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


//set up your FastLED to your configuration ps: the more pins the better
    FastLED.addLeds<WS2812, 12>(leds, NUM_LEDS);

    pinMode(PIN_RECORD,INPUT_PULLDOWN);

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
          }
      }
    myFile=SD.open("/filename",FILE_WRITE);
    artnet.setLedsBuffer((uint8_t*)leds); //set the buffer to put the frame once a frame has been received this is mandatory
    artnet.setframeRecordCallback(&recordfunction); //this is not mandatory
    if(sd_card_present)
        artnet.startArtnetrecord(myFile);
    artnet.begin(NUM_LEDS,UNIVERSE_SIZE); //configure artnet

}

void loop() {
// put pin PIN_RECORD  to HIGH to start the  record
// put it back to LOW to stop it
  while(record_status && sd_card_present){

              has_recorded=true;
              artnet.readFrameRecord();
              record_status=digitalRead(PIN_RECORD);
          }

  if(has_recorded && sd_card_present)
  {
      artnet.stopArtnetRecord();
  }
  record_status=digitalRead(PIN_RECORD);
}
