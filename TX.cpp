#include <Arduino.h>  
#include <SPI.h> 
#include <RF24.h>

// Create an instance of the RF24 class with CE pin 4 and CSN pin 5
RF24 radio(4, 5); 

// Define the address for the communication pipe
const byte address[6] = "00001";

// ---- Packet struct ----
struct Packet { 
    uint32_t seq;
    uint16_t testID;
    uint8_t mode;
}; 

Packet pkt; 

unsigned long rttMin = 0xFFFFFFFF;
unsigned long rttMax = 0;
unsigned long long rttSum = 0;
uint32_t rttCount = 0;

uint32_t seq_count = 0; 
uint32_t packetsCreated = 0;
uint32_t packetsSent = 0;
uint32_t ackSuccess = 0;

void setup() { 

Serial.begin(115200);
delay(1000); // give serial monitor time to connect
 
if (!radio.begin()) {
    Serial.println("Radio hardware not responding!");
    while (1) {
        // IF radio initialization was NOT successful, print an error and stop the program.
    }
  }


radio.setPALevel(RF24_PA_LOW);      // low power for testing
radio.setDataRate(RF24_250KBPS);    // start at slowest rate
radio.setChannel(76);               // pick any clear channel, 0-125 
radio.setRetries(5, 15);            // (delay, count) 
radio.openWritingPipe(address);     // TX writes to this address
radio.stopListening();              // NRF is in TX mode

  Serial.println("TX ready.");

}

void loop() { 

    pkt.seq = seq_count; 
    pkt.testID = 1;
    pkt.mode = 0;   
    packetsCreated++;

    unsigned long t0 = micros(); //Records time in microseconds before sending the packet (RTT) start
    bool sent = radio.write(&pkt, sizeof(pkt)); //Returns true if an ACK came back, false after all retries failed.
    unsigned long rtt = micros() - t0; //Current time - the start time, how long did ACK 
    packetsSent++;

    uint8_t retryCount = radio.getARC(); // auto retry count  

    if (sent) {
        ackSuccess++;  

        if (rtt < rttMin) {
            rttMin = rtt;
        }

        if (rtt > rttMax) {
            rttMax = rtt;
        }

        rttSum += rtt; 
        rttCount++;

    }

    Serial.print("Sent seq=");
    Serial.print(pkt.seq); //Prints the seq number

    if(sent) 
    {
        Serial.print(" ACK OK");
    } 
    else
    {
        Serial.print(" failed");
    }

    Serial.print(" | RTT(us)="); 
    Serial.println(rtt); //Prints the round trip time in microseconds

    Serial.print(" | retries=");
    Serial.println(retryCount);

    // --- periodic summary every 20 packets ---
  if (seq_count % 20 == 0) {
    float ackRate = ((float)(100.0 * ackSuccess) / packetsSent);
    float rttAvg = (float)rttSum / rttCount : 0;

    Serial.println("--- TX SUMMARY ---");
    Serial.print("Created=");
    Serial.print(packetsCreated);
    Serial.print(" Sent=");
    Serial.print(packetsSent);
    Serial.print(" ACKsuccess=");
    Serial.print(ackSuccess);
    Serial.print(" (");
    Serial.print(ackRate);
    Serial.println("%)");

    Serial.print("RTT(us) min=");
    Serial.print(rttMin);
    Serial.print(" RTT(us) max=");
    Serial.print(rttMax);
    Serial.print(" RTT(us) avg=");
    Serial.println(rttAvg);
    Serial.println("------------------");
  }

    seq_count++;
    delay(500); // 2 packets/sec

}
