
  // FleetLink LoRa Radio Code
  // RKing 3/24/16
  
    #include <SPI.h>
    #include <RH_RF95.h> // this is my modified version with sendWithDelay()
    
    #define FREQUENCY  915.000 // 
    #define LED           15 // Moteino MEGAs have LEDs on D15

    #define LORA_PAYLOAD_LENGTH 16 // set to avoid dropping longer packet problem
    #define SERIAL1_BUFFER_LENGTH 64 // set by serial buffer size between Imp and Moteino
 

    RH_RF95 rf95;
    
  ////////////////////////////////////////////
  // readLoRA variables
  ////////////////////////////////////////////
  
    uint8_t messageToImp[SERIAL1_BUFFER_LENGTH] = {0};  
    int messageToImpPointer = 0;
    bool messageToImpComplete = false;   
  
    uint8_t loRaRxPacket[LORA_PAYLOAD_LENGTH] = {0}; 
    uint8_t loRaRxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa RX function to return number of byte

    // where we put characters received from Imp by Serial1 and to be sent to LoRa radio       
    uint8_t impRxByte = 0; // byte buffer for Serial1 read  

  ////////////////////////////////////////////    
  // readSerial variables    
  ////////////////////////////////////////////  
  
    uint8_t messageToLoRa[SERIAL1_BUFFER_LENGTH] = {0};  
    int messageToLoRaPointer = 0;
    bool messageToLoRaComplete = false;   
    
    uint8_t loRaTxPacket[LORA_PAYLOAD_LENGTH] = {0} ;
    uint8_t loRaTxPacketLength = LORA_PAYLOAD_LENGTH;    // need a variable for LoRa TX function in uint8_t form
 
    unsigned long serial1RxTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
    unsigned long serial1RxTimeout = 30; // max amount of time to assume the Imp will take to send a byte
   
    int loRaTxDelay = 0; // desired delay
 
    unsigned long networkActivityTimer; //in ms, we use 'unsigned long' because ms timer is unsigned long integer
    unsigned long networkActivityTimeout = 5000; // clear the network out every N seconds of no activity

  ////////////////////////////////////////////    
  // change the values below to match what's being written to the radio in the setup below
  ////////////////////////////////////////////  
      
    float SF = 12;
    float BW = 250000;   
    float symbolPeriod = (1000*pow(2,SF))/BW; // in milliseconds
    
   
  

    
  void setup() 
  {
    Serial.begin(115200);  // Serial0 bus for programming and monitor
    Serial1.begin(115200); // Serial1 bus for talking to Electric Imp 
    pinMode(LED, OUTPUT);   
    
    rf95.init(); 
    if (!rf95.init()) {
      Serial.println("init failed");
    } else { 
    Serial.println("init OK - "); }
    Serial.print("symbol period ");
    Serial.println(symbolPeriod);
 
    rf95.setFrequency(FREQUENCY);
    rf95.spiWrite(RH_RF95_REG_0B_OCP, 0x33);     // Set OCP to 160 mA..KEEP DUTY CYCLE LOW at +20 dBm!!
    rf95.spiWrite(RH_RF95_REG_0C_LNA, 0x03);     // LNA settings: Boost on, 150% LNA curren
    rf95.spiWrite(RH_RF95_REG_1D_MODEM_CONFIG1, 0x82);    // Set BW to 250 kHz, Coding Rate to 4/5, Explicit Header ON
    rf95.spiWrite(RH_RF95_REG_1E_MODEM_CONFIG2, 0xA4);     // Set SF at 10, TXContinous = Normal, CRC ON 
    rf95.spiWrite(RH_RF95_REG_21_PREAMBLE_LSB, 0x0A); // Preamble Length LSB (+ 4.25 Symbols)       
    rf95.spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, 0x10); // Payload length in bytes. 
    rf95.spiWrite(RH_RF95_REG_26_MODEM_CONFIG3, 0x04); // LNA gain set by the internal AGC loop
    rf95.setTxPower(20); 
    // rf95.printRegisters();  // this will print out the LoRa registers for debug
    
  }


  void loop() {
      delay(10);
      if (rf95.available())   {
          networkActivityTimer = millis();
          if (rf95.recv(loRaRxPacket, &loRaRxPacketLength)) {
              Serial.print("recvd: "); 
              for (int i = 0; i< loRaRxPacketLength; i++) {
                if (loRaRxPacket[i] != 10) {
                    Serial.print((char)loRaRxPacket[i]); 
                }
              }  
              Serial.print(" at "); Serial.print(millis());
              int lastRSSIVal = rf95.lastRssi();
              Serial.print(" with RSSI= "); Serial.println(lastRSSIVal); 
              for (int i = 0; i < LORA_PAYLOAD_LENGTH; i++) {             
                  messageToImp[messageToImpPointer] = loRaRxPacket[i]; 
                  if (loRaRxPacket[i] == 10) {   // complete if this message contains the EOL character ('/N') or the message buffer is full 
                      messageToImpComplete = true; 
                    }
                  if (messageToImpPointer >= SERIAL1_BUFFER_LENGTH) {
                      messageToImpComplete = true;
                    }  
                messageToImpPointer++;
              } 
              if (messageToImpComplete) {  
                  Serial1.write((char*)messageToImp); // send the LoRa reception to IMP
                  Serial.println("");  
                  clearAll(); 
              }
          }
      }  
   
      
                
      if (Serial1.available()) {
          networkActivityTimer = millis();
          serial1RxTimer =  millis();
          
          while (millis() - serial1RxTimer < serial1RxTimeout) { 
            
            
              if (Serial1.available()) {            
                    impRxByte = Serial1.read(); // get one byte from serial port
                    messageToLoRa[messageToLoRaPointer] = impRxByte;
                    if (impRxByte == 10) {  // complete if this is the EOL character ('/N') or the message buffer is full
                        messageToLoRaComplete = true;
                    }
                    if (messageToLoRaPointer >= SERIAL1_BUFFER_LENGTH-1) {
                        messageToLoRaComplete = true;
                    }
                    if (messageToLoRaComplete) {  // If complete, send message to packet parser for LoRa TX
                        int loRaTxDelay =  ((String(messageToLoRa[0]).toInt() - 48) * 10) + (String(messageToLoRa[1]).toInt() - 48); // stupid, don't even ask!
                        loRaTxDelay = min(loRaTxDelay, 99); // put bounds on it in case of TX garbage
                        loRaTxDelay = max(loRaTxDelay, 0);
                        int delayQuantum = 4*symbolPeriod; 
                        loRaTxDelay = delayQuantum*loRaTxDelay; 
                        int numberFullTxPackets = (messageToLoRaPointer-1) / LORA_PAYLOAD_LENGTH;
                        int numberPartialTxPackets = 0;
                        if (((messageToLoRaPointer-1) % LORA_PAYLOAD_LENGTH) != 0) { 
                            numberPartialTxPackets =1;
                         }
                        int numberTxPackets = numberFullTxPackets + numberPartialTxPackets;
                        bool txCanceledFlag = false;
                        for (int i=0; i<numberTxPackets; i++) {
                            clearloRaTxPacket(); // Clear the loRaTx packet to prep for next one
                            if (!txCanceledFlag) {
                                if (i != 0) {
                                    loRaTxDelay = 0; // 
                                }       
                                Serial.print("send ");
                                for (int j=0; j<LORA_PAYLOAD_LENGTH; j++) {
                                    int k = min(((i*LORA_PAYLOAD_LENGTH) + j + 2),SERIAL1_BUFFER_LENGTH-1);  
                                    loRaTxPacket[j] = messageToLoRa[k]; 
                                    if (loRaTxPacket[j] != 10) {
                                        Serial.print((char)loRaTxPacket[j]);
                                    }
                                 } 
                                Serial.print("  with delay  "); Serial.print(loRaTxDelay); Serial.print(", start  "); Serial.print(millis()); 
                                                                
                                digitalWrite(LED,HIGH);
                                if(rf95.sendIfNoCad(loRaTxPacket, loRaTxPacketLength, loRaTxDelay)) { 
                                } else {  // A CAD occurred, indicating another unit won the TX collision and our packet was not sent
                                   txCanceledFlag = true;
                                   Serial.print(", cancel "); Serial.print(millis()); 
                                   digitalWrite(LED,LOW); 

                                }
                                rf95.waitPacketSent(); // block until complete
                                Serial.print(", end   "); Serial.println(millis()); 
                                
                                digitalWrite(LED,LOW); 
                          }
                       }
                    }  else {
                        messageToLoRaPointer++;  // if we're not done getting bytes, increment the pointer
                    }
              } 
          }
          
          Serial.println();
          clearAll(); 

      }
      if (millis() - networkActivityTimer > networkActivityTimeout) {
          networkActivityTimer = millis();
          clearAll();    
      }
  }      




  void clearAll() {
    clearMessageToImp();
    clearloRaTxPacket();
    clearMessageToLoRa();
    Serial.println("clear");
  }

  
  void clearMessageToImp() {
      for( int i = 0; i < SERIAL1_BUFFER_LENGTH;  ++i ) {
          messageToImp[i] = (char)0;
      }
      messageToImpPointer = 0; // reset the pointer
      messageToImpComplete = false; // and reset complete flag
  }  

  void clearloRaTxPacket() {
      for(int i = 0; i < LORA_PAYLOAD_LENGTH;  ++i ) {
          loRaTxPacket[i] = (char)0;
  } }
  

  void clearMessageToLoRa() {
      for( int i = 0; i < SERIAL1_BUFFER_LENGTH;  ++i ) {
          messageToLoRa[i] = (char)0;
      }
      messageToLoRaPointer = 0; // reset the pointer
      messageToLoRaComplete = false; // and reset complete flag
  } 
    
    
    
// Moteino code to support FleetLink Plan D demo
// This version breaks all comms into 16 byte payloads to overcome long packet issues
// Added CAD detect and sendWithDelay
// based on UML diagram for Radio Code of 3/18/16

// All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]
// we break up the messageToLoRA into 16B packets for transmission  
// we may need up to 4 packets max

// serial buffer on moteino is 63 bytes, so that's the hard limit on transferring things    
// without worrying about reading things fast enough to empty buffer before it fills up

// Data to transmit over LoRa; keep short to avoid transmission problems.  
// 16 bytes takes 1/2 second or so
// All LoRa packets are defined to be LORA_PAYLOAD_LENGTH [16B]     

// BW=250kHz and SF=10  gives us 1200 bps (which is our eventual bitrate after amplifier is available) for betwork development
// BW=250 kHZ, SF=12 gives us our 800 meter range for outdoor testing pre-amplifier, but only 300 bps
// BW=500 kHz, SF=11 is our end game with amp: 800 meter range at 1200 bps

// Default Preamble length is 8 bits, but I increased it to 10 bits to allow CAD detect to bail on a TX
// collision and still return to RX mode and catch the colliding transmission.  This is necessary to allow
// possible future network behaviors where in range units can pick up a failed forwarding and retransmit later

// Set the payload length to 16B beacuse 64B packets were dropping regularly and I found an app note that 
// suggested shorter was more reliable

// calculate the symbol period (used to define TX delay period)
// from http://www.semtech.com/images/datasheet/an1200.22.pdf

// delay extraction
// the first two characters of the message is the number of delay quantum we want
// The CAD effort takes 2-3 symbol periods, it appears from experiment
// therefore, I somewhat arbitrarily set the delay time to have a quantization of 3 symbol periods, so they would be mostly distinct

// load up the 2D loRaTxPackets array for transmission
// There are N-2 characters in the message after the delay code.  This includes a '/N' at the end
// We know the MessagePointer is pointing at the last byte (0 based addressing), 
// so we'll need 
//    ((MessagePointer + 1 {0 based] - 2 [delay characters] ) DIV Payload Size) full packets -- since these are integers, "/" is DIV
//   +  ((MessagePointer + 1 {0 based] - 2 [delay characters] ) MOD Payload Size) fractional packet] --"%" is Modulo
// example:  MessagePointer is 17 --> 18 total characters --> 16 charcaters to transmit after delay removed
// requires 1 full packet and 0 fractionals

// the first two bytes specifies the transmission delay, so we strip them off and don't send via radio
// don;t extend past the length os the serial buffer!
