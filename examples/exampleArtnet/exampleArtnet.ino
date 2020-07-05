

#include <ArtnetESP32.h>

#include "FastLED.h"
FASTLED_USING_NAMESPACE

//The following has to be adapted to your specifications
#define LED_WIDTH 123
#define LED_HEIGHT 48
#define NUM_LEDS LED_WIDTH*LED_HEIGHT
#define UNIVERSE_SIZE 170 //my setup is 170 leds per universe no matter if the last universe is not full.
CRGB leds[NUM_LEDS];


ArtnetESP32 artnet;


void displayfunction()
{
  if (artnet.frameslues%100==0)
   Serial.printf("nb frames read: %d  nb of incomplete frames:%d lost:%.2f %%\n",artnet.frameslues,artnet.lostframes,(float)(artnet.lostframes*100)/artnet.frameslues);
   //here the buffer is the led array hence a simple FastLED.show() is enough to display the array
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
    FastLED.addLeds<NEOPIXEL, 12>(leds, NUM_LEDS);

    artnet.setFrameCallback(&displayfunction); //set the function that will be called back a frame has been received
    artnet.setLedsBuffer((uint8_t*)leds); //set the buffer to put the frame once a frame has been received

    artnet.begin(NUM_LEDS,UNIVERSE_SIZE); //configure artnet


}

void loop() {
    artnet.readFrame(); //ask to read a full frame
}
