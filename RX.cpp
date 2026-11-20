#include <stdint.h>
#include <SPI.h>
#include <RF24.h>

// ---- Radio setup ----
RF24 radio(4, 5); // CE, CSN — same pins as TX, but on the RX board

// Must match TX exactly
const byte address[6] = "00001";

// Must match TX exactly, field for field
struct Packet {
  uint32_t seq;
  uint16_t testID;
  uint8_t mode;
};

Packet pkt;
uint32_t lastSeq = 0;
bool firstPacket = true;

// ---- Counters ----
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;
uint32_t missingCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    while (1) {
        // IF radio initialization was NOT successful, print an error and stop the program.
    }
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.openReadingPipe(0, address); // pipe 0, same address as TX
  radio.startListening();            // RX mode

  Serial.println("RX ready.");
}

void loop() {
  if (radio.available()) {
    radio.read(&pkt, sizeof(pkt));

    if (firstPacket) {
      // first packet ever — just accept it, nothing to compare against
      uniqueCount++;
      lastSeq = pkt.seq;
      firstPacket = false;
    }
    else if (pkt.seq == lastSeq) {
      duplicateCount++;
      Serial.print("DUPLICATE seq=");
      Serial.println(pkt.seq);
    }
    else if (pkt.seq > lastSeq + 1) {
      uint32_t gap = pkt.seq - lastSeq - 1;
      missingCount += gap;
      uniqueCount++;
      Serial.print("GAP: missing ");
      Serial.print(gap);
      Serial.print(" | now at seq=");
      Serial.println(pkt.seq);
      lastSeq = pkt.seq;
    }
    else {
      uniqueCount++;
      lastSeq = pkt.seq;
      Serial.print("Received seq=");
      Serial.println(pkt.seq);
    } 
    
    float PDR = (100 * (float)uniqueCount / (float)(uniqueCount + missingCount)); 
    float PacketLoss = 100.00 - PDR ; 
    

    // print running totals every packet (fine for now; you'll throttle this later)
    Serial.print("  [unique=");
    Serial.print(uniqueCount);
    Serial.print(" dup=");
    Serial.print(duplicateCount);
    Serial.print(" missing=");
    Serial.print(missingCount);
    Serial.println("]");

    Serial.print("  PDR=");
    Serial.print(PDR);
    Serial.println("%"); 

    Serial.print("  Packet Loss=");
    Serial.print(PacketLoss);
    Serial.println("%"); 
  }
}