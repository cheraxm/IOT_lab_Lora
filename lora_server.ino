#include <SPI.h>
#include <RH_RF95.h>
#include <math.h>

#define RFM95_CS 10
#define RFM95_RST 7
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match TX's freq!
#define RF95_FREQ 903.08

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// LoRa parameters
const uint8_t spreadingFactor = 12;  // Set to 12 to match the transmitter
const uint32_t bandwidth = 125000;  // 125 kHz bandwidth
const uint8_t codingRate = 5;  // 4/5 coding rate

float calculateToA(uint8_t spreadingFactor, uint32_t bandwidth, uint16_t payloadLength) {
    const float preambleLength = 8;  // 8 symbols by default
    const float explicitHeaderEnable = 1;  // 0 for implicit header, 1 for explicit header
    const float lowDataRateOptimize = (spreadingFactor >= 11) ? 1 : 0;

    // Calculate symbol duration (ms)
    float Ts = (1 << spreadingFactor) / (bandwidth / 1000.0);

    // Calculate number of symbols in the payload
    float payloadSymbNb = 8 + max(ceil((8 * payloadLength - 4 * spreadingFactor + 28 + 16 - 20 * explicitHeaderEnable) / (4 * (spreadingFactor - 2 * lowDataRateOptimize))) * (codingRate + 4), 0);

    // Calculate total number of symbols
    float numSymbols = preambleLength + 4.25 + payloadSymbNb;

    // Calculate time on air
    float timeOnAir = numSymbols * Ts;

    return timeOnAir;
}

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Arduino LoRa RX Test!");
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Set Spreading Factor to 12
  rf95.setSpreadingFactor(spreadingFactor);
  Serial.print("Set Spreading Factor to: "); Serial.println(spreadingFactor);
  
  // Set Bandwidth to 125 kHz
  rf95.setSignalBandwidth(bandwidth);
  Serial.print("Set Bandwidth to: "); Serial.println(bandwidth);
  
  // Set Coding Rate to 4/5
  rf95.setCodingRate4(codingRate);
  Serial.print("Set Coding Rate to: 4/"); Serial.println(codingRate);

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      // Calculate Time on Air for the received message
      float timeOnAir = calculateToA(spreadingFactor, bandwidth, len);
      
      Serial.print("Received packet (");
      Serial.print(len);
      Serial.print(" bytes) ");
      RH_RF95::printBuffer("Contents: ", buf, len);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      Serial.print("Estimated Time on Air: ");
      Serial.print(timeOnAir);
      Serial.println(" ms");
      
      // Send a reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      
      // Calculate and print Time on Air for the reply
      float replyTimeOnAir = calculateToA(spreadingFactor, bandwidth, sizeof(data));
      Serial.print("Reply Time on Air: ");
      Serial.print(replyTimeOnAir);
      Serial.println(" ms");
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}
