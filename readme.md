# cub2bed_server
## What is cub2bed_server? 
Simple two-way message appliance

### Purpose
Client message appliance alerts server appliance of message and receives status updates from server

### Contributors

### Software Dependencies
- uses Radiohead http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.100.zip
  - adafruit maintains a GitHub branch at https://github.com/adafruit/RadioHead
- Adafruit NeoPixel https://github.com/adafruit/Adafruit_NeoPixel


### BOM
- 1X each: Adafruit Arduino Pro Mini 328 - 5V (part 2378) + Adafruit FRM69HCW Radio Breakout (part 3070) + Adafruit FTDI Friend (part 284)
or
- 1X: Adafruit Feather M0 RFM69HCW Packet Radio (part 3176)
- 1X: NeoPixel
- 2x: button
- 1 protoboard
- wire

### Pinouts
- see Fritzing diagram

### Information Sources
- Adafruit RFM69HCW (SPI): https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts?view=all
- https://learn.adafruit.com/adafruit-feather-m0-radio-with-rfm69-packet-radio
- Radiohead main page: https://www.airspayce.com/mikem/arduino/RadioHead/index.html
- https://community.particle.io/t/my-attempt-at-rf95-lora/40573/3
- https://groups.google.com/forum/#!forum/radiohead-arduino
- https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts?view=all

### Issues
- 051120: seems to fall out of sync where X will do something but Y will not be in a state to respond?
- 051920: implement robust error handling; how does the user know what state they are in (visually, programmatically)
- 051920: need to remove delay() loops in code, might be causing sync issues, debug message also gated by delay()

### Questions
- 041919: do I buffer overrun or truncate when inbound packet is larger than defined buffer len?
- 051920: can I control the timeout on the Reliable Datagram send calls, or do I care as long as the routine is called enough? (aka, unless one of the radios is pooched or out of range, a packet will eventually get to the other node)
- 051920: how to implement a button based reset of the board (for when they fall out of sync)
- 051920: Should we alert or debug log when RSSI is below a described db?

### Learnings

### Feature Requests
- 051920: Implement app level packet counts, transmitted to both parties, to help implement out-of-sync fixes
- 051920: Create standardized packet, set buffers to that size rather than protocol max (saves memory)
- 051920: Use the builtin LED as a crude indicator (packet send, receive, etc.)
 
### Revisions
- 050320: first version, adapted from datagram_server
- 050520: [Q 041919] code looks like it could send packets to multiple clients based on form of client_address declare? -> yes
- 050920
	- implemented conditional messaging
	- code is running on Feather M0 RFM69HCW
- 080220
	- conditional visual alert code for NEOPIXEL and RELAY
	- [FR]051920: Capture RSSI in DEBUG for all transmissions