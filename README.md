#  Artnet ESP32 library
===========

This library is a take on the usage of the Artnet protocol to display leds and record on a SD using an esp32 when dealing with a large number of leds (> 800). This library will allow you to:
* Live play of Artnet stream
* Record the stream on a SD card
* Replay the recorded file

## Couple of considerations when using Artnet 

### Receiving Artnet universes
The Artnet protocol is intensively used in lights shows and  numerous lighting software have the capability to output the data using the artnet protocol. The artnet protocol uses the concept of universe. A universe is a subset of the information you wanne send. Each universe can contain 512 bytes of data + the header information.
The Artnet protocol uses UDP internet protocol which does not provided receive/check/resend capabilities which means that when the artnet server sends out an artnet packet it doesn't wait for any acknowledgment before sending the next one. As a consequence a packet not intercepted  by the artnet reciever is LOST.

If you send RGB pixel informatiion you can have maximum 512/3 =170 pixels per universes. 
Hence if your project contains 1800 leds you would need at least 11 universes (10 full universes and the last one with 100 pixels).

Now if you have an artnet broadcast at 25fps (each frame is 40ms appart) each universe will be maximum 4/11=0.36ms appart.
Hence speed is critical in gathering the universes without loosing any artnet packet.

### Using the universes 
For most users the use of artnet is the following :
```C
Gather the artnet universes ->move the data in the leds buffer-> display the leds
  ^                                                                   |
  |___________________________________________________________________|
```
The WS281X are one of the most  leds used. This led are cheap and easy to use. But they are clockless leds which means that the speed to 'upload' the data in them is fixed and in that case the 'upload speed' is 800kHz  which means that to transfer the data for one led it requires 3x8bits/800.000=30us. Hence in our case 1800 x 30us=54ms.

If using a sequentiel flow when displaying  you will lose 2 artnet frames (the first one totally and part of the second frame => not usable)
It is necessary to reduce the time needed to display the leds => split your leds over several PINS. If you're using the Fastled library it will divide the time by the number of pins used. Hence if you are using 4 pins (450 leds per pin) the display time will be 13.5ms hence loosing only one frame.

it will work but the display will not be smooth.

### Recording/Replaying  the universes
Writing/Reading to/from a SD card is also a time consuming process which could last longer than time betwwen two frames. The frames are not always sent with an exact timing depending on the artnet sending program. 
To be able to replay it with the most accurate timing the program will store in microseconds the delay between frames to be used during replay.

### Solutions implemented in this library

#### Artnet universe processing speed
To process the wifi packet and the artnet packet this library uses the barebone socket functions to reduce the  processing. 
It gather all the universes at once removing all unnessary code out of this function

#### Using the second core
If we use a pure sequential workflow the program will not be listening to artnet packet while displaying the leds. Hence this library implements the use of the two cores of the ESP32 : one core is processing the data while the other one is displaying/recording the data. 


### Take away
When using artnet streaming, You need to take into account the timing issues : 
* Do not try to get artnet to broadcast at too much fps otherwise you will have too many lost frames in artnet
* Dispatch your leds over several pins to lower the refresh time of your display
* Do not hesitate to buy a good SD card to have a good write/read speed

## Library usage

```C

```


