/*
  Project Name:   cub2bed_server
  Developer:      Eric Klein Jr. (temp2@ericklein.com)
  Description:    server for cub2bed solution

  See README.md for target information, revision history, feature requests, etc.
*/

#define DEBUG       // Output to the serial port

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

// unique addresses for each client, can not be server address
#define MY_ADDRESS 1

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
    Serial.print((int)RF69_FREQ);
    Serial.println("MHz. This node is server.");
  #endif
}

// Dont put this on the stack:
uint8_t data[] = "ROGER WILCO";
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop() {
  if (rf69_manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got packet from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);

      // Send a reply back to the originator client
      if (!rf69_manager.sendtoWait(data, sizeof(data), from))
        Serial.println("Sending failed (no ack)");
    }
  }
}