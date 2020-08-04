/*
  Project Name:   cub2bed_server
  Developer:      Eric Klein Jr. (temp2@ericklein.com)
  Description:    server for cub2bed solution

  See README.md for target information, revision history, feature requests, etc.
*/

#define DEBUG       // Output to the serial port
#define NEOPIXEL    // output to neopixel(s)
#define RELAY

// Remember: pins 3,4,8 are reserved for radio

//button
#include <buttonhandler.h>
#define onTheWayButtonPin     11
#define needToWorkButtonPin   10
#define longButtonPressDelay  3000
ButtonHandler buttonOnTheWay(onTheWayButtonPin,longButtonPressDelay);
ButtonHandler buttonNeedToWork(needToWorkButtonPin, longButtonPressDelay);
// globals related to buttons
enum { BTN_NOPRESS = 0, BTN_SHORTPRESS, BTN_LONGPRESS };

#ifdef RELAY
  #define relay_trigger_PIN 12
#endif

#ifdef NEOPIXEL
  #include <Adafruit_NeoPixel.h>
  #define neoPixelPin   9
  #define ledCount      1
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledCount, neoPixelPin);
#endif

// 900mhz radio
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
// Conditional code compile for radio pinout
#if defined(ARDUINO_SAMD_ZERO)
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
#endif

#if defined (__AVR_ATmega328P__)
  #define RFM69_INT     3
  #define RFM69_CS      4
  #define RFM69_RST     2
#endif

// server address
#define MY_ADDRESS      1
// unique addresses for each client, can not be server address
#define CLIENT_ADDRESS  2
// Instantiate radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);
// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);
// 400-[433]-600 for LoRa radio, 850-[868,915]-950 for 900mhz, must match transmitter
#define RF69_FREQ 915.0
// 14-20dbm for RFM69HW power
#define RF69_POWER 20
// who am i? (server address)

void setup() 
{
  #ifdef DEBUG
    Serial.begin(115200);
    while (!Serial) 
      {
        delay(1);
      }
      Serial.println("cub2bed server started");
      #ifdef RELAY
        Serial.print("RELAY");
      #endif
      #ifdef NEOPIXEL
        Serial.print(" NEOPIXEL");
      #endif
      Serial.println(" code path(s) enabled");
  #endif

  // radio setup
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

   // radio reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69_manager.init())
  {
    #ifdef DEBUG
      Serial.println("RFM69 radio init failed");
    #endif
    while (1);
  }
  
  // Default after radio init is 434.0MHz
  if (!rf69.setFrequency(RF69_FREQ))
  {
    #ifdef DEBUG
      Serial.println("RFM69 set frequency failed");
    #endif
    while (1);
  }
  
  // Defaults after radio init are modulation GFSK_Rb250Fd250, +13dbM (for low power module), no encryption
  // For RFM69HW, 14-20dbm for power, 2nd arg must be true for 69HCW
  rf69.setTxPower(RF69_POWER, true);

  // The encryption key has to match the transmitter
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
  // radio successfully set up
  #ifdef DEBUG
    Serial.print("RFM69 radio OK @ ");
    Serial.println((int)RF69_FREQ);
  #endif

  // Setup push buttons
  buttonNeedToWork.init();
  buttonOnTheWay.init();

  #ifdef RELAY
    pinMode(relay_trigger_PIN, OUTPUT);
  #endif

  #ifdef NEOPIXEL
    // initialize neopixel
    strip.begin();
    // green = nothing to see, move along
    //strip.setPixelColor(0,0,255,0);
    //strip.setBrightness(50);
    strip.show();
  #endif
}

bool  requestFromClient = false;
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop()
{
  // if radio is ready and we're not already handling a client request
  if ((rf69_manager.available()) && (!requestFromClient))
  {
    // Wait for a message from a client
    uint8_t len = sizeof(buf);
    uint8_t from; 
    if (rf69_manager.recvfromAck(buf, &len, &from))
    {
      #ifdef DEBUG
        Serial.print("Got packet from #");
        Serial.print(from);
        Serial.print(" [RSSI :");
        Serial.print(rf69.lastRssi());
        Serial.print("] : ");
        Serial.println((char*)buf);
      #endif
      // block request processing until resolved
      requestFromClient = true;
      buf[len] = 0; // zero out remaining string
      // visually indicate that we have a user request
      #ifdef NEOPIXEL
        strip.setPixelColor(0,255,255,0); // yellow
        strip.show();
      #endif
      #ifdef RELAY
        // turn on light
        digitalWrite(relay_trigger_PIN, HIGH);
      #endif
      #ifdef DEBUG
        Serial.println("Need to respond to client request");
      #endif
    }
  }
  resolveButtons();
}

void resolveButtons()
{
  switch (buttonOnTheWay.handle()) 
  {
    case BTN_SHORTPRESS:
      #ifdef DEBUG
        Serial.print("short press of on-the-way button; ");
      #endif
      // process pending requests only
      if (requestFromClient)
      {
        uint8_t data[] = "ontheway";
        //send response to client
        if (!rf69_manager.sendtoWait(data, sizeof(data), CLIENT_ADDRESS))
        {
          #ifdef DEBUG
            Serial.print(" RSSI= ");
            Serial.println(rf69.lastRssi());
            Serial.println("; Sending failed (no ack)");
          #endif
        }
        else
        {
          // release processing block and return to waiting
          requestFromClient = false;
          #ifdef RELAY
            digitalWrite(relay_trigger_PIN, LOW);
          #endif
          #ifdef NEOPIXEL
            // blink green to indicate success
            for (int i=0;i<10;i++)
            {
              strip.setPixelColor(0,0,255,0); // green
              strip.show();
              delay(100);
              strip.setPixelColor(0,0,0,0);
              strip.show();
              delay(100);
            }
            // deactivate alert light
            strip.setPixelColor(0,0,0,0);
            strip.show();
          #endif
          #ifdef DEBUG
            Serial.println("resolved");
            Serial.print("RSSI= ");
            Serial.print(rf69.lastRssi());
          #endif
        }
      }
      else
      {
        #ifdef DEBUG
          Serial.println("ignored");
        #endif
      }
    break;
    case BTN_LONGPRESS:
      #ifdef DEBUG
        Serial.println("long press of on-the-way button");
      #endif
    break;
  }
  switch (buttonNeedToWork.handle()) 
  {
    case BTN_SHORTPRESS:
      #ifdef DEBUG
        Serial.print("short press of need-to-work button; ");
      #endif
      // process pending requests only
      if (requestFromClient)
      {
        uint8_t data[] = "needtowork";
        //send response to client
        if (!rf69_manager.sendtoWait(data, sizeof(data), CLIENT_ADDRESS))
        {
          #ifdef DEBUG
            Serial.print(" RSSI= ");
            Serial.println(rf69.lastRssi());
            Serial.println("; Sending failed (no ack)");
          #endif
        }
        else
        {
          // release processing block and return to waiting
          requestFromClient = false;
          #ifdef RELAY
            digitalWrite(relay_trigger_PIN, LOW);
          #endif
          #ifdef NEOPIXEL
            // blink red to indicate success
            for (int i=0;i<10;i++)
            {
              strip.setPixelColor(0,255,0,0); // red
              strip.show();
              delay(100);
              strip.setPixelColor(0,0,0,0);
              strip.show();
              delay(100);
            }
            // deactivate alert light
            strip.setPixelColor(0,0,0,0);
            strip.show();
          #endif
          #ifdef DEBUG
            Serial.println("resolved");
            Serial.print("RSSI= ");
            Serial.print(rf69.lastRssi());
          #endif
        }
      }
      else
      {
        #ifdef DEBUG
          Serial.println("ignored");
        #endif
      }
    break;
    case BTN_LONGPRESS:
      #ifdef DEBUG
        Serial.println("long press of need-to-work button");
      #endif
    break;
  }
}