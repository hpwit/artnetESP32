
#include <WiFi.h>
//#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <Artnet.h>
#include "FastLED.h"


Artnet artnet;
void setup() {

  
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
  // put your setup code here, to run once:
  
artnet.begin(123*48,170); //the number of pixels and the maximum size of your iniverses
}

void loop() {
  artnet.read();
  /* in artnet.getframe() you have the content of the frame
  for instance you can do
  memcopy(leds,artnet.getframe(),nbpixels*sizeof(CRGB));
  */
 FastLED.show();
  artnet.resetsync();
  // put your main code here, to run repeatedly:

}
