
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

uint32_t record_duration2=0;
bool sd_card_present=false;

void afterSDread()
{
  FastLED.show();
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
    myFile=SD.open("/filename");
    artnet.setLedsBuffer((uint8_t*)leds); //set the buffer to put the frame once a frame has been received this is mandatory
    artnet.setreadFromSDCallback(&afterSDread);
    artnet.begin(NUM_LEDS,UNIVERSE_SIZE); //configure artnet

}

void loop() {
    if(sd_card_present)
    {
      if (!artnet.readNextFrameAndWait(myFile))

              {
                record_duration2=millis()-record_duration2;
                  myFile.seek(0);

                  Serial.printf("duration %ld \n",record_duration2);

                  record_duration2=millis();

              }
    }
}
