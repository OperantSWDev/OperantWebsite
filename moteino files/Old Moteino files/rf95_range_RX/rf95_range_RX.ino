  // Starting point for Moteino code to support FleetLink Plan D demo
  // RKing 2/15/16
  // Works only with Moteino Mega LoRa
  
  #include <SPI.h>
  #include <RH_RF95.h>
  
  #define FREQUENCY  915.000 // May need to tweak this slightly, unit by unit until we get 500 kHz BW working
  #define LED           15 // Moteino MEGAs have LEDs on D15
  
  // Initialize global variables
  uint8_t expectedMessageLength = 63;
  bool receivedRX = false;
  int rxCounter = 0;
    
  // Singleton instance of the radio driver
  RH_RF95 rf95;
  
  
  // Set up things
  
  void setup() 
  {
    Serial.begin(115200);  // Serial0 bus for programming and monitor
    Serial1.begin(9600); // Serial1 bus for talking to Electric Imp 
    
    rf95.init(); // get radio ready
    if (!rf95.init())
    Serial.println("init failed");
    else { Serial.print("init OK - "); Serial.print(FREQUENCY); Serial.println("mhz"); }
    Serial.println("--------------------------------");
    Serial.println();
    Serial.println();
    
    rf95.setFrequency(FREQUENCY);
    
    // Follows are my attempt to increase range - RK
    rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);  // This is the long range config
    rf95.spiWrite(RH_RF95_REG_0B_OCP, 0x2F); //set OCP to 120 mA..KEEP DUTY CYCLE LOW at +20 dBm!!
    rf95.spiWrite(RH_RF95_REG_0C_LNA, 0x03); // LNA settings: boost on, gains set by AGC is 0x03
    rf95.setTxPower(20); // set to +3 for testing, +20 for field use
    rf95.printRegisters();  // this will print out the LoRa registers for debug
  }



  void loop()
  {  
 
 
     if (rf95.waitAvailableTimeout(5900))  
        { 
          uint8_t messageFromLoRa[62] = {0};
          
          if (rf95.recv(messageFromLoRa, &expectedMessageLength)) {

            Serial.print("RX ");
            Serial.print(rxCounter);
            Serial.print("  ");
            Serial.print((char*)messageFromLoRa);
            int lastRSSIVal = rf95.lastRssi();
            Serial.print("   RSSI ");
            Serial.println(lastRSSIVal); 
            receivedRX = true;
            rxCounter++;
            
       // clear buffer
       
          } else  {
            Serial.println("RX failed");
            }
       
        } 
         if (!receivedRX) {
          Serial.println("-----------////////LoRa TIMEOUT///////-------------");
         }
         receivedRX = false;
         
    Blink(LED,3);

  }


  void Blink(byte PIN, int DELAY_MS)
  {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
  }


