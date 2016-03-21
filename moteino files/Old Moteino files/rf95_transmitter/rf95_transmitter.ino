  // Starting point for Moteino code to support FleetLink Plan D demo
  // RKing 2/15/16
  // Works only with Moteino Mega LoRa
  
  #include <SPI.h>
  #include <RH_RF95.h>
  
  #define FREQUENCY  915.00 // May need to tweak this slightly, unit by unit until we get 500 kHz BW working
  #define LED           15 // Moteino MEGAs have LEDs on D15
  
  // Initialize global variables


    char incomingChar = 0; //  buffer for Serial read
    char disposableChar = 0;
     
    uint8_t messageFromLoRa[100] = {"0"};
    uint8_t messageFromLoRaMaxLength = 120;
   
    int messageFromImpPointer = 0;  // pointer for Serial read 
    uint8_t messageFromImp[120] ;
    uint8_t messageToLoRa1[60] = {0};
    uint8_t messageToLoRa2[60] = {0};
    
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
    
    rf95.setFrequency(FREQUENCY);
    
    // Follows are my attempt to increase range - RK
    rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);  // This is the long range config
    rf95.spiWrite(RH_RF95_REG_0B_OCP, 0x2F); //set OCP to 120 mA..KEEP DUTY CYCLE LOW at +20 dBm!!
    rf95.spiWrite(RH_RF95_REG_0C_LNA, 0x03); // LNA settings: boost on, gains set by AGC is 0x03
    rf95.setTxPower(3); // set to +3 for testing, +20 for field use
    rf95.printRegisters();  // this will print out the LoRa registers for debug
  }



  void loop()
  {  
    //Serial.print(" loop ");
   //Make sure the LoRa message buffers aren't full
   if (messageFromImpPointer >= 120) {
      Serial.println("Imp RX buffer full");
      // If it is, clear the buffer  
      for( int i = 0; i < sizeof(messageFromImp);  ++i ) {
      messageFromImp[i] = (char)0;
      }
      messageFromImpPointer = 0; // reset the pointer
      Serial.println("Imp RX buffer cleared");
    }  
    


    // This next section is where we read items from the serial1 bus (i.e. Electric Imp) and when message is complete, 
    // pass it on to the LoRa radio for broadcast
    
    if (Serial1.available()) { 
       incomingChar = Serial1.read(); // get one byte from serial port
       messageFromImp[messageFromImpPointer++] = incomingChar;  // add it to end of message
             //Serial.print(messageFromImp );    
             Serial.print(incomingChar );
       switch(incomingChar) {
         // if the received byte is New Line character '/N', then you have the whole message and should send it to the radio
         // otherwise, keep looping the main loop
         case 10:  
             Serial.print("IMP sent ");  
             Serial.print((char*)messageFromImp );
            
             for( int j = 0; j < sizeof(messageToLoRa1);  ++j ) {
                  messageToLoRa1[j] = messageFromImp[j];
                  messageToLoRa2[j] = messageFromImp[j + sizeof(messageToLoRa1)];
             /*     Serial.print(j);
                  Serial.print(" - ");
                  Serial.print((char)messageToLoRa1[j]);

                   Serial.print(" - ");
                   Serial.println((char)messageToLoRa2[j]);*/
            }  
                                  Serial.println("Sending packet 1");
                                      Blink(LED,3);
             rf95.send(messageToLoRa1, sizeof(messageToLoRa1)); // off to the radio!
             rf95.waitPacketSent(); // block until complete
                                  Serial.println("Sending packet 2");
                                      Blink(LED,3);
             rf95.send(messageToLoRa2, sizeof(messageToLoRa2)); // off to the radio!
             rf95.waitPacketSent(); // block until complete
                                    Serial.println("Packet 1&2 sent");
                                        Blink(LED,3);
               
            // Serial.println("broadcast by LoRa complete" );
            // clear the Imp RX buffer
             for( int k = 0; k < sizeof(messageFromImp);  ++k ) {
                    messageFromImp[k] = (char)0;
              }    
              Serial.print("Imp RX buffer is now: ");
              Serial.println((char*)messageFromImp);
            // reset the transmit buffer pointer
            messageFromImpPointer = 0;
            Serial.println("Imp RX buffer pointer reset after send");
            Serial.println("--------------------------------");
             Serial.println();
             Serial.println();
            
         break;
         }         
     }






    // This section checks the LoRa radio buffer to see if anything has arrived
    // Will block for timeout period waiting for LoRa
    // Can't block too long or serial1 buffer might overflow (that's why its throttled to 9600 baud 
  
     if (rf95.waitAvailableTimeout(5))  
        { 
           //          Serial.print((char*)messageFromLoRa);
          if (rf95.recv(messageFromLoRa, &messageFromLoRaMaxLength))
          {
            Serial.print("LoRa received ");
            Serial.print((char*)messageFromLoRa);
            int lastRSSIVal = rf95.lastRssi();
            Serial.print("RSSI: ");
            Serial.println(lastRSSIVal); 
            // Handler for LoRa received messages goes here 
            Serial1.write((char*)messageFromLoRa); // upload the LoRa reception to IMP
            Serial1.print("RSSI=");
            Serial1.print(lastRSSIVal);
            
            Serial.println("send to IMP complete");
            // clear the LoRa RX buffer
           // Serial.println(sizeof(messageFromLoRa));
            for( int m = 0; m < sizeof(messageFromLoRa);  ++m ) {
                  messageFromLoRa[m] = (char)0;
                        //         Serial.println(i);
            }    
             Serial.println("LoRa RX buffer cleared");
             Serial.print("LoRa RX buffer is now: ");
             Serial.println((char*)messageFromLoRa);
             Serial.println("--------------------------------");
             Serial.println();
             Serial.println();
             
             for( int n = 0; n <= 64;  ++n ) {
                  messageFromLoRa[n] = (char)0;
                  disposableChar = Serial1.read(); // get one byte from serial port and throuw it away
                  // if you've been off receiving LoRa messages, you may have received IMP comms which you have to throw away 
                  // (since its likely incomplete)
            }   
          }
          else
          {
              Serial.println("RX failed");
            }
       
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


