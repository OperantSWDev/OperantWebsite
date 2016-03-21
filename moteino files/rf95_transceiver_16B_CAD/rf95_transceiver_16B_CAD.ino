  // Moteino code to support FleetLink Plan D demo
  // This version breaks all comms into 16 byte payloads to overcome long packet issues
  // Added CAD detect and sendWithDelay
  // based on UML diagram for RAdio Code of 3/18/16
  // RKing 3/18/16
  // Works only with Moteino Mega LoRa
  
    #include <SPI.h>
    #include <RH_RF95.h> // this is my modified version with sendWithDelay()
    
    #define FREQUENCY  915.000 // 
    #define LED           15 // Moteino MEGAs have LEDs on D15

    // Data to transmit over LoRa; keep short to avoid transmission problems.  
    // 16 bytes takes 1/2 second or so
    #define LORA_PAYLOAD_LENGTH 16 // set to avoid dropping longer packet problem
    #define SERIAL1_BUFFER_LENGTH 63 // set by serial buffer size between Imp and Moteino
 

  ////////////////////////////////////////////
  // readLoRA variables
  ////////////////////////////////////////////
  
    // where we put characters received from LoRa and to be sent (via Seria1) to Imp
    // serial buffer on moteino is 63 bytes, so that's the hard limit on transferring things    
    // without worrying about reading things fast enough to empty buffer before it fills up
    
    uint8_t messageToImp[SERIAL1_BUFFER_LENGTH] = {0};  
    int messageToImpPointer = 0;
    bool messageToImpComplete = false;   
  
    // All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]     
    uint8_t loRaRxPacket[LORA_PAYLOAD_LENGTH] = {0}; 
    uint8_t loRaRxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa RX function to return number of byte

    // where we put characters received from Imp by Serial1 and to be sent to LoRa radio       
    uint8_t impRxByte = 0; // byte buffer for Serial1 read
   
    // A timer is set when a LoRa packet is received, when it times out, we assume reception is complete\
    unsigned long loRaRxTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
    unsigned long loRaRxTimeout = 300; // max amount of time to assume a LoRa RX will take to complete between packets and processing
    

  ////////////////////////////////////////////    
  // readSerial variables    
  ////////////////////////////////////////////  
  
    // where we put entire message received from Serial1 (from Imp) and to be sent to LoRa radio
    uint8_t messageToLoRa[SERIAL1_BUFFER_LENGTH] = {0};  
    int messageToLoRaPointer = 0;
    bool messageToLoRaComplete = false;   
    
    // All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]
    // we break up the messageToLoRA into 16B packets for transmission  
    // we may need up to 4 packets max
    uint8_t loRaTxPacketArray[4][LORA_PAYLOAD_LENGTH] = {0} ;
    uint8_t loRaTxPacket[LORA_PAYLOAD_LENGTH] = {0} ;
    uint8_t loRaTxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa TX function in uint8_t form
 
    // A timer is set when a Serial1 byte is received from Imp, if it timesout, we assume the Imp is done and send the packet
    unsigned long serial1RxTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
    unsigned long serial1RxTimeout = 300; // max amount of time to assume the Imp will take to send 16B packet
   
    int loRaTxDelay = 0; // desired delay in 10 millisecond quantum steps, 0 to 2550 ms
    unsigned long loRaTxDelayTimer = 0;
    
    
    // Singleton instance of the radio driver
    RH_RF95 rf95;
    
    // Network activity timer
    unsigned long networkActivityTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
    unsigned long networkActivityTimeout = 5000; // clear the network out every 3 seconds of no activity
      
  

  void setup() 
  {
    Serial.begin(115200);  // Serial0 bus for programming and monitor
    Serial1.begin(115200); // Serial1 bus for talking to Electric Imp 

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
    rf95.spiWrite(RH_RF95_REG_1E_MODEM_CONFIG2, 0xC4); // set SF at 12, TXContinous = Normal, CRC ON                 <===!! DEVELOPMENT
    rf95.spiWrite(RH_RF95_REG_21_PREAMBLE_LSB, 0x08); // Preamble Length LSB (+ 4.25 Symbols)
    rf95.spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x10); // Payload length in bytes. Set at 16B by definition 
    rf95.spiWrite(RH_RF95_REG_26_MODEM_CONFIG3, 0x04); // LNA gain set by the internal AGC loop
    rf95.setTxPower(3); // set to +3 for testing, +15 fpr general data use;  +20 for range test use                  <===!! DEVELOPMENT
    rf95.setModeRx(); // turn on receiver (probably not necessary, but I was missing first messages...so...)
    // rf95.printRegisters();  // this will print out the LoRa registers for debug
 
    networkActivityTimer = millis(); //initialize the network activity timer

  }


  void loop() {
      
 
    
      // check the LoRa reception flag
      if (rf95.available())   {
        
          // readLoRa 
          
          // reset the network activity timer
          networkActivityTimer = millis();
    
          // as long as the time out hasn't expired, only look for LoRa packets and ignore Serial1
          // setLoRaRxTimer
          loRaRxTimer = millis();
                   
          while (millis() - loRaRxTimer < loRaRxTimeout) { 
              // recv
              if (rf95.recv(loRaRxPacket, &loRaRxPacketLength)) {
                
                  
                  //debug printing
                  Serial.print("LoRa packet received: ");
                  Serial.print((char*)loRaRxPacket); 
                  int lastRSSIVal = rf95.lastRssi();
                  Serial.print(" with RSSI= ");
                  Serial.println(lastRSSIVal); 
                  
                  
                  // add MessageToImp
                  // add received packet to message buffer to send to Imp
                  for (int i = 0; i < LORA_PAYLOAD_LENGTH; i++) {             
                      messageToImp[messageToImpPointer] = loRaRxPacket[i];
                      // if this is the EOL character ('/N') or the message buffer is full , then send the message to Imp
                      if (loRaRxPacket[i] == 10) {
                          messageToImpComplete = true;
                      }
                      if (messageToImpPointer >= SERIAL1_BUFFER_LENGTH) {
                          messageToImpComplete = true;
                    }  
                    messageToImpPointer++;
                  } 
               
                  // sendToImp
                  if (messageToImpComplete) {  // If complete, send message to Imp
                      Serial1.write((char*)messageToImp); // send the LoRa reception to IMP

                      //debug printing
                      Serial.print("Message sent to Imp = "); // echo for debug
                      Serial.println((char*)messageToImp); // send the LoRa reception to IMP
                      
                      clearAll(); 
                  }
              }
          }
      }  
   
   
   
   
                
      // check the Serial1 buffer to seee if Imp is sending things     
      if (Serial1.available()) {
          // readSerial
          rf95.setModeIdle(); // immediately go to Idle mode, so we don't receive any confusing packets         

          // reset the network activity timer
          networkActivityTimer = millis();
          
          // as long as the time out hasn't expired, only look for Serial1 packets and ignore LoRa
          // setSerial1Timer
          serial1RxTimer =  millis();
          
          while (millis() - serial1RxTimer < serial1RxTimeout) { 

              if (Serial1.available()) {            
                    //Serial.println("byte at serial1 port..");
                    impRxByte = Serial1.read(); // get one byte from serial port
                    
                    /*
                    //debug printing
                    Serial.print(impRxByte);
                    Serial.print(" pointer ");
                    Serial.print(messageToLoRaPointer);
                    Serial.println();
                    */
                    
                    // add MessageToLoRa
                    // add received byte to message buffer to send to LoRa
                    messageToLoRa[messageToLoRaPointer] = impRxByte;
                    
                    // if this is the EOL character ('/N') or the message buffer is full, 
                    // then send the message to packet parser and then radi     
                    if (impRxByte == 10) {
                        messageToLoRaComplete = true;
                    }
                    if (messageToLoRaPointer >= SERIAL1_BUFFER_LENGTH-1) {
                        messageToLoRaComplete = true;
                    }

                    // send to PacketParser
                    if (messageToLoRaComplete) {  // If complete, send message to packet parser for LoRa TX
      
                        //debug printing
                        Serial.print("Message received from Serial1 = "); // echo for debug
                        Serial.println((char*)messageToLoRa); // send the LoRa reception to IMP
                        
                        // packetParser
                        
                        // the first two characters of the message is the number of 50 milliscond delay quantum we want
                        int loRaTxDelay =  ((String(messageToLoRa[0]).toInt() - 48) * 10) + (String(messageToLoRa[1]).toInt() - 48); // stupid, don't even ask
                        loRaTxDelay = min(loRaTxDelay,19); // put some bounds on it in case of TX garbage
                        loRaTxDelay = max(loRaTxDelay, 1);
                        loRaTxDelay = 50*loRaTxDelay; //comes in 50 ms quantum
                        Serial.print("requested TX delay= ");
                        Serial.println(loRaTxDelay);
                        
                        // load up the 2D loRaTxPackets array for transmission
                        int numberTxPackets = ((messageToLoRaPointer-1)/LORA_PAYLOAD_LENGTH) + 1; // we'll need this many TX packets
                        //Serial.print("number of TX packets = ");
                        //Serial.println(numberTxPackets);
                        
                        for (int i=0; i<numberTxPackets; i++) {
                            for (int j=0; j<LORA_PAYLOAD_LENGTH; j++) {
                                int k = i*LORA_PAYLOAD_LENGTH + j;
                                loRaTxPacketArray[i][j]=messageToLoRa[k+2]; // the first two bytes specifies the transmission delay, so we strip them off and don't send via radio
                            }
                        }    
                        
                        // send off to LoRa radio for transmission after delay
                        bool txCanceledFlag = false;
                        for (int i=0; i<numberTxPackets; i++) {
                          
                        clearloRaTxPacket(); // Clear the loRaTx packet to prep for next one
                             
                             for (int j=0; j<LORA_PAYLOAD_LENGTH; j++) {
                                  int k = i*LORA_PAYLOAD_LENGTH + j;
                                  loRaTxPacket[j] = loRaTxPacketArray[i][j]; // load the TX packet buffer
                                  //Serial.println(loRaTxPacket[j]);
                              }    
      
                              //debug printing
                              //Serial.print("LoRa Packet Length= ");
                              //Serial.println(sizeof(loRaTxPacket));
                              
                              
                              
                              digitalWrite(LED,HIGH);

                              if (i == 0) { // the first packet through
                                 if(rf95.sendIfNoCad(loRaTxPacket, loRaTxPacketLength, loRaTxDelay)) {; // did we send first packet with specified delay?
                                    rf95.waitPacketSent(); // block until complete
                                    Serial.print("LoRa packet [");
                                    Serial.print(i);
                                    Serial.print("] sent: ") ; 
                                    for (int n=0; n<sizeof(loRaTxPacket); n++) {
                                      Serial.print((char)loRaTxPacket[n]); 
                                    }
                                    Serial.println(" sent successfully");
                                 } else {
                                   Serial.println("TX canceled");
                                   txCanceledFlag = true;
                                 }
                              
                              } else { // send 2nd through 4th packets immediately unless the whole transmission was canceled
                                   if (!txCanceledFlag) {
                                     rf95.send(loRaTxPacket, loRaTxPacketLength); // off to the radio
                                     rf95.waitPacketSent(); // block until complete
                                     Serial.print("LoRa packet [");
                                     Serial.print(i);
                                     Serial.print("] sent: ") ; 
                                     for (int n=0; n<sizeof(loRaTxPacket); n++) {
                                       Serial.print((char)loRaTxPacket[n]); 
                                     }
                                     Serial.println(" sent successfully");
                                   }
                              }
                              digitalWrite(LED,LOW);
                              
                        }
                        
                        // done with transmission, clear everything
                        clearAll(); 
                        
                    }  else {
                        messageToLoRaPointer++;  // if we're not done getting bytes, increment the pointer
                    }

              } 
          
          }
          rf95.setModeRx(); // return to RX mode as we exit the Serial read loop    
      }
      
      
      
      
      // clear the network buffers if nothings happened lately on network
      if (millis() - networkActivityTimer > networkActivityTimeout) {
          networkActivityTimer = millis();
          clearAll();    
      }
    
    
  }      



  void clearAll() {
    clearMessageToImp();
    clearloRaTxPacket();
    clearloRaTxPacketArray();
    clearMessageToLoRa();
    Serial.println("clear all");
  }
  
  
  void clearMessageToImp() {
      for( int i = 0; i < SERIAL1_BUFFER_LENGTH;  ++i ) {
          messageToImp[i] = (char)0;
          }
      messageToImpPointer = 0; // reset the pointer
      messageToImpComplete = false; // and reset complete flag
      //Serial.println("messageToImp cleared");
    }  


  void clearloRaTxPacket() {
      for(int i = 0; i < LORA_PAYLOAD_LENGTH;  ++i ) {
            loRaTxPacket[i] = (char)0;
            }
      //Serial.println("loRaTxPacket cleared");
  }
          
                    
  void clearloRaTxPacketArray() {
          for (int i=0; i<4; i++) {
              for (int j=0; j<LORA_PAYLOAD_LENGTH; j++) {
                loRaTxPacketArray[i][j] = (char)0; 
                }
            }
      //Serial.println("loRaTxPacketArray cleared");
  }

  
  void clearMessageToLoRa() {
      for( int i = 0; i < SERIAL1_BUFFER_LENGTH;  ++i ) {
          messageToLoRa[i] = (char)0;
          }
      messageToLoRaPointer = 0; // reset the pointer
      messageToLoRaComplete = false; // and reset complete flag
      //Serial.println("messageToLoRa cleared");

    } 
