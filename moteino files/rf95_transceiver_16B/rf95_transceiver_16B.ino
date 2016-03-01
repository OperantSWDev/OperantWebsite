  // Moteino code to support FleetLink Plan D demo
  // This version breaks all comms into 16 byte payloads to overcome long packet issues
  // RKing 3/1/16
  // Works only with Moteino Mega LoRa
  
  #include <SPI.h>
  #include <RH_RF95.h>
  
  #define FREQUENCY  915.000 // May need to tweak this slightly, unit by unit until we get 500 kHz BW working
  #define LED           15 // Moteino MEGAs have LEDs on D15

   // Data to transmit over LoRa; keep short to avoid transmission problems.  
   // 16 bytes takes 1/2 second or so
   #define LORA_PAYLOAD_LENGTH 16
   #define SERIAL1_BUFFER_LENGTH 63
 
  // Initialize global variables

    // where we put characters received from LoRa and to be sent to Imp
    // serial buffer on moteino is 63 bytes, so that's the hard limit on transferring things    
    // without worrying about reading things fast enough to empty buffer befor eit fills
    uint8_t messageToImp[SERIAL1_BUFFER_LENGTH] = {0};  
  
    int messageToImpPointer = 0;
    bool messageToImpComplete = false;   
    
    // where we put characters received from Imp by Serial1 and to be sent to LoRa radio
    // All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]       
    uint8_t loRaTxPacket[LORA_PAYLOAD_LENGTH] = {0} ;
    uint8_t loRaTxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa TX function in uint8_t form
  
    int loRaTxPacketPointer = 0;  // pointer for Serial read
  
    // where we put packets received from LoRa radio
    // All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]     
    uint8_t loRaRxPacket[LORA_PAYLOAD_LENGTH] = {0}; 
    uint8_t loRaRxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa RX function to return number of byte


    // where we put characters received from Imp by Serial1 and to be sent to LoRa radio       
    uint8_t impRxByte = 0; // byte buffer for Serial1 read
   
   // A timer is set when a LoRa packet is received, when it timesout, we assume reception is complete
   unsigned long loRaRxTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
   unsigned long loRaTimeout = 3000; // max amount of time to assume a LoRa RX will take to complete between packets and processing
   
    // Singleton instance of the radio driver
    RH_RF95 rf95;
  
  
  // Set up things
  
  void setup() 
  {
    Serial.begin(115200);  // Serial0 bus for programming and monitor
    Serial1.begin(9600); // Serial1 bus for talking to Electric Imp 

    pinMode(LED, OUTPUT);   
   
    
    rf95.init(); // get radio ready
    if (!rf95.init()) {
      Serial.println("init failed");
    } else { Serial.print("init OK - "); Serial.print(FREQUENCY); Serial.println("mhz"); }
    Serial.println("--------------------------------");
    Serial.println();

    rf95.setFrequency(FREQUENCY);
    
    // Follows are my attempt to increase range - RK   
    rf95.spiWrite(RH_RF95_REG_0B_OCP, 0x2F); //set OCP to 120 mA..KEEP DUTY CYCLE LOW at +20 dBm!!
    rf95.spiWrite(RH_RF95_REG_0C_LNA, 0x03); // LNA settings: Boost on, 150% LNA current
    rf95.spiWrite(RH_RF95_REG_1D_MODEM_CONFIG1, 0x82); //set BW to 250 kHz, Coding Rate to 4/5, Explicit Header ON
    rf95.spiWrite(RH_RF95_REG_1E_MODEM_CONFIG2, 0xC4); // set SF at 12, TXContinous = Normal, CRC ON 
    rf95.spiWrite(RH_RF95_REG_21_PREAMBLE_LSB, 0x08); // Preamble Length LSB (+ 4.25 Symbols)
    rf95.spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x10); // Payload length in bytes. Set at 16B by definition 
    rf95.spiWrite(RH_RF95_REG_26_MODEM_CONFIG3, 0x04); // LNA gain set by the internal AGC loop
    rf95.setTxPower(20); // set to +3 for testing, +15 fpr general data use;  +20 for range test use
    rf95.setModeRx(); // turn on receiver (probably not necessary, but I was missing first messages...so...)
    rf95.printRegisters();  // this will print out the LoRa registers for debug
  }



  void loop() {  

        //         Serial.println("Entering main loop..");   // <--------------------------
                 
    // block 10 ms and wait for LoRa reception..if none, go around main loop again
    // might need to tweak this wait.  It seems necessary to have  a wait, but you don't
    // want to let Imp RX serial buffer fill up and miss anything either
        if (rf95.waitAvailableTimeout(10))   {  
          
            if (rf95.recv(loRaRxPacket, &loRaRxPacketLength)) {
                 
                /*
                //debug printing
                Serial.print("LoRa packet received: ");
                Serial.print((char*)loRaRxPacket); 
                int lastRSSIVal = rf95.lastRssi();
                Serial.print(" with RSSI= ");
                Serial.println(lastRSSIVal);  
                */
                
                
                loRaRxTimer = millis(); // set timer when a reception occurs  
               
                // add received packet to message buffer to send to Imp
                // if this is the EOL character ('/N') or the message buffer is full , then send the message to Imp
                for (int i = 0; i < LORA_PAYLOAD_LENGTH; i++) {             
                    messageToImp[messageToImpPointer] = loRaRxPacket[i];
                    if (loRaRxPacket[i] == 10) {
                        messageToImpComplete = true;
                    }
                    if (messageToImpPointer >= SERIAL1_BUFFER_LENGTH) {
                        messageToImpComplete = true;
                      }  
                    messageToImpPointer++;
                  } 
                 
                if (messageToImpComplete) {  // If complete, send message to Imp
                    Serial1.write((char*)messageToImp); // send the LoRa reception to IMP
                    
                    //Serial.print("Message sent to Imp = "); // echo for debug
                    //Serial.println((char*)messageToImp); // send the LoRa reception to IMP
                    clearMessageToImp(); 
                }
                
                
             }
          }    
                
        // if the LoRa RX timer has expired (which means there hasn't been a LoRa reception recently, 
        // go check the Serial1 buffer to seee if Imp is sending things     
        
 
        long  currentRxTimer =  millis() - loRaRxTimer;

         
        if (currentRxTimer > loRaTimeout) {
            
            if (Serial1.available()) {
              
                impRxByte = Serial1.read(); // get one byte from serial port
            
                // increment pointer and add it to the packet to be sent to LoRa 
                loRaTxPacket[loRaTxPacketPointer] = impRxByte;
                loRaTxPacketPointer++;
                
                // if packet is complete (either 16B long or terminated with /N character)
                // then send it out via LoRa
                    if (loRaTxPacketPointer >= LORA_PAYLOAD_LENGTH || impRxByte == 10) {
                      
                      //
                      // NEED TO ADD CARRIER DETECT BAIL OUT HERE <-------------!!!!!!!!!!!!!!!!!!!!!
                      //
                        digitalWrite(LED,HIGH);
                        rf95.send(loRaTxPacket, loRaTxPacketLength); // off to the radio!
                        rf95.waitPacketSent(); // block until complete
                        digitalWrite(LED,LOW);

                      /*
                         Serial.print("LoRa packet sent: " );        
                         Serial.println((char*)loRaTxPacket );                  
                      */
                      
                        clearloRaTxPacket(); // Clear the loRaTx packet and pointer
            } 
          }  
      
      clearMessageToImp(); // clear the Imp buffer because it's stale
      
      Blink(LED,3);

   }
  }



  void Blink(byte PIN, int DELAY_MS) {


        delay(DELAY_MS);
        digitalWrite(PIN,LOW);
      }

  void clearloRaTxPacket() {
      for(int i = 0; i < LORA_PAYLOAD_LENGTH;  ++i ) {
            loRaTxPacket[i] = (char)0;
            }
      loRaTxPacketPointer = 0; // reset the pointer 
    }
  
  
  void clearMessageToImp() {
      for( int i = 0; i < SERIAL1_BUFFER_LENGTH;  ++i ) {
          messageToImp[i] = (char)0;
          }
      messageToImpPointer = 0; // reset the pointer
      messageToImpComplete = false; // and reset complete flag
    }  

