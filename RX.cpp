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

unsigned long testStartTime = 0;
unsigned long lastGoodPacketTime = 0;
unsigned long longestOutage = 0;
uint32_t totalPayloadBytes = 0; 

// ---- RPD sampling ----
unsigned long lastRpdSampleTime = 0;
uint32_t rpdSampleCount = 0;
uint32_t rpdActiveCount = 0;

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

  testStartTime = millis();
  lastGoodPacketTime = testStartTime; 
  lastRpdSampleTime = testStartTime;

  Serial.println("RX ready.");
}

void loop() {

    // --- RPD sampling: runs every loop, independent of packet arrival ---
  unsigned long nowRpd = millis();
  if (nowRpd - lastRpdSampleTime >= 10) {
    bool rpd = radio.testRPD();
    rpdSampleCount++;
    if (rpd) rpdActiveCount++;
    lastRpdSampleTime = nowRpd;
  }

  if (radio.available()) {
    radio.read(&pkt, sizeof(pkt)); 

    unsigned long now = millis();

    // Track outage — time since the last successfully received packet
    unsigned long gapTime = now - lastGoodPacketTime;
    if (gapTime > longestOutage) {
      longestOutage = gapTime;
    }
    lastGoodPacketTime = now;

// Goodput — only count payload bytes from non-duplicate packets
if (pkt.seq != lastSeq || firstPacket) {
  totalPayloadBytes += sizeof(pkt); // real payload would use sizeof(solarData) once that's added
}

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
    unsigned long elapsedMs = now - testStartTime; 
    float goodputBps = (elapsedMs > 0) ? (totalPayloadBytes * 8.0 * 1000.0 / elapsedMs) : 0;
    float rpdActivity = (rpdSampleCount > 0) ? (100.0 * rpdActiveCount / rpdSampleCount) : 0;

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

    Serial.print("  Goodput(bps)=");
    Serial.print(goodputBps);

    Serial.print(" | LongestOutage(ms)=");
    Serial.println(longestOutage);

    Serial.print("  RPD Activity=");
    Serial.print(rpdActivity);
    Serial.println("%");
  }
}