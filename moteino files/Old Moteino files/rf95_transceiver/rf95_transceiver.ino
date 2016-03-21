  // Moteino code to support FleetLink Plan D demo
  // RKing 2/15/16
  // Works only with Moteino Mega LoRa
  
  #include <SPI.h>
  #include <RH_ASK.h>
  #include <RH_RF95.h>
  
  #define FREQUENCY  915.000 // May need to tweak this slightly, unit by unit until we get 500 kHz BW working
  #define LED           15 // Moteino MEGAs have LEDs on D15
  
  // Initialize global variables

    uint8_t messageFromImpPointer = 0;  // pointer for Serial read
    uint8_t incomingByte = 0; // byte buffer for Serial read
     
    uint8_t messageFromImp[62] = {0};  // where we put characters received from Imp by Serial1 and to be sent to LoRa radio
    uint8_t messageFromImpLength = 62;    

    String subsetMessageFromImp; // first five characters are all the fleetlink commnads, so we want to just check those soemtimes    
    String subsetMessageFromLoRa; // first five characters are all the fleetlink commnads, so we want to just check those soemtimes    
     
    uint8_t rangeModeInterestPacket[5] = {105,47,102,47,114}; // this is "i/f/r----" in char codes and repesents the range test mode interest
    uint8_t rangeModeDataPacket[10] = {100,47,102,47,114,47, 40,0,0,0}; // this is "d/f/r" in char codes and repesents the range test mode data, 4 spaces saved for RSSI
    
     // Dont put this on the stack:
    uint8_t buf[61];
//RH_ASK_MAX_MESSAGE_LEN
    
  // Singleton instance of the radio driver
  RH_RF95 rf95;
  
  
  // Set up things
  
  void setup() 
  {
    Serial.begin(115200);  // Serial0 bus for programming and monitor
    Serial1.begin(9600); // Serial1 bus for talking to Electric Imp 
    
    rf95.init(); // get radio ready
    if (!rf95.init()) {
      Serial.println("init failed");
    } else { Serial.print("init OK - "); Serial.print(FREQUENCY); Serial.println("mhz"); }
    Serial.println("--------------------------------");
    Serial.println();
    
    rf95.setFrequency(FREQUENCY);
    
    // Follows are my attempt to increase range - RK
    rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);  // This is the long range config
    rf95.spiWrite(RH_RF95_REG_0B_OCP, 0x2F); //set OCP to 120 mA..KEEP DUTY CYCLE LOW at +20 dBm!!
    rf95.spiWrite(RH_RF95_REG_0C_LNA, 0x03); // LNA settings: boost on, gains set by AGC is 0x03
    rf95.setTxPower(15); // set to +3 for testing, +15 fpr general data use;  +20 for range test use
    rf95.setModeRx(); // turn on receiver (probably not necessary, but I was missing first messages...so...)
    rf95.printRegisters();  // this will print out the LoRa registers for debug


  }



  void loop()
  {  
   
   
   
   //Make sure the Imp RX message buffer isn't full
     if (messageFromImpPointer > 61) {
        Serial.println("Imp RX buffer full");
        // If it is, clear the buffer  
        for( int i = 0; i < 62;  ++i ) {
        messageFromImp[i] = (char)0;
        }
        messageFromImpPointer = 0; // reset the pointer
        Serial.println("Imp RX buffer cleared");
      }  


    // This next section is where we read items from the serial1 bus (i.e. Electric Imp) and when message is complete, 
    // pass it on to the LoRa radio for broadcast
    
    if (Serial1.available()) {
    // Serial.print("messageFromImpPointer= ");
    // Serial.println(messageFromImpPointer);
 
     incomingByte = Serial1.read(); // get one byte from serial port
    //     Serial.print(" byte= ");
    // Serial.println(incomingByte);
     
    messageFromImp[messageFromImpPointer] = incomingByte;  // add it to end of message
     messageFromImpPointer++;
     
     // if the received byte is New Line character '/N', then you have the whole message and should send it to the radio
     // otherwise, keep looping the main loop
     switch(incomingByte) {

           case 10: 
           
             /* Serial.print("IMP sent ");  
             // Serial.print((char*)messageFromImp );   // it's a uint8_t so convert to char for printing
    
             // range test interest packets "i/f/r" are special and are responded to at the Moteino level 
             // the reason is we normally have a 63 byte message, but for range testing shorter packets give better and faster results
             // so we look for them specically: have to turn the uint8_t needed by radio to s string for comaprison
             // if we find a range test packet request, we send it by LoRa in a short message (and expect to get a short prely form neighbors*/
                
                 Serial.print("Imp sent: ");
                 Serial.print((char*)messageFromImp);
                          
                 subsetMessageFromImp = ""; // clear the local command test string for next time
      
                
                for( int q=0; q<=4; q++) {
                  subsetMessageFromImp += (char)messageFromImp[q];
                }
                //Serial.println(subsetMessageFromImp);
                if (subsetMessageFromImp == "i/f/r") {
                  // full power for range mode
                   rf95.setTxPower(20);
                   delay(100);
                   Serial.println("TX set to hi power");
                   //
                         rf95.send(rangeModeInterestPacket, 5); // send the special short range interest packet
                         rf95.waitPacketSent(); // block until complete
                         Serial.println(" special range packet broadcast by LoRa complete" );
                         
                   // back to normal power 
                   rf95.setTxPower(15);
                   Serial.println("TX set to normal power");
                   //
                   
                }  else {
                        if(messageFromImpPointer>5){
                          // send a regular packet 
                         rf95.send(messageFromImp, 62); // off to the radio!
                         rf95.waitPacketSent(); // block until complete
                         Serial.println(" full packet broadcast by LoRa complete" );
                        } else {
                          Serial.println("malformed interest received from IMP"); // seemd like I'm ending up with empty packets from Imp, not sure how
                        }
                }
                        Serial.println("-----------");
               // clear the LoRa RX buffer
               for( int j = 0; j < 62;  ++j ) {
                      messageFromImp[j] = (char)0;
                }    
                //  Serial.print("Imp RX buffer is now: ");
               //   Serial.println((char*)messageFromImp);
                // reset the transmit buffer pointer
                messageFromImpPointer = 0;
                Serial.println("LoRa RX buffer pointer reset after send");
               // Serial.println("--------------------------------");
           break;

       }         
     }

    ////////////////////////////////////////////////////////////////////////////////////////
    // This section checks the LoRa radio buffer to see if anything has arrived
    // Will block for timeout period waiting for LoRa
    // Can't block too long or serial1 buffer might overflow (that's why its throttled to 9600 baud 
    ////////////////////////////////////////////////////////////////////////////////////////
    
     if (rf95.waitAvailableTimeout(10))   { 
           uint8_t len =sizeof(buf)  ;


 

               //  Serial.println(messageFromLoRaLength);  
            if (rf95.recv(buf, &len)) {
                 Serial.print("LoRa rcvd: ");
                 Serial.print((char*)buf); 
                 Serial.print(" of length: ");
                 Serial.println(len);                 
            int lastRSSIVal = rf95.lastRssi();
                 Serial.print(" RSSI= ");
                 Serial.println(lastRSSIVal);
  
            // range test interest packets "i/f/r" are special and are responded to at the Moteino level 
            // the reason is we normally have a 63 byte message, but for range testing shorter packets give better and faster results
            // so we look for them specically: have to turn the uint8_t needed by radio to s string for comaprison
            // if we find a range test packet request, we send it by LoRa in a short message (and expect to get a short prely form neighbors
                      
            subsetMessageFromLoRa = ""; // clear the local command test string for next time
            for( int r=0; r<=4; r++) {
              subsetMessageFromLoRa += (char)buf[r];
            }
            
            if (subsetMessageFromLoRa == "i/f/r") {
                 uint8_t rssiArray[4] = {0};
                 // add the last RSSI value to the returned data packet
                 for (int s=0; s <=3; s++) {
                     rangeModeDataPacket[6+s] = String(lastRSSIVal).charAt(s);
                 }
                 rf95.send(rangeModeDataPacket, sizeof(rangeModeDataPacket)); // right back out to the radio!
                 rf95.waitPacketSent(); // block until complete    
            } else {
                //clear the Imp RX buffer first, because if we just got a message to send to it, we don't want any stray bits hanging out before the Imp response
               for( int i = 0; i < 62;  ++i ) {
                  messageFromImp[i] = (char)0;
                  }
                messageFromImpPointer = 0; // reset the pointer
                Serial.println("Imp RX buffer cleared"); 
                
                // Send it up to the Imp for processing 
                Serial1.write((char*)buf); // upload the LoRa reception to IMP
                Serial.println("LoRa msg sent to IMP"); 
            }
                
            // clear the LoRa RX buffer
            for( int k = 0; k < 62;  k++ ) {
                buf[k] = (char)0;
                }
              
              Serial.println("LoRa RX buffer cleared");              
            }  else {
              Serial.println("RX failed");
            }
            Serial.println("-----------");
        } 
        
      Blink(LED,3);

  }

  void Blink(byte PIN, int DELAY_MS)
  {
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
  }


