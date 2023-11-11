/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example sends chunk data with ack to the recieving unit (recieve_chunks_with_ack).

Step1.
Set compileFor328P OR compileForEsp8266 OR compileForEsp32 OR compileForEsp32Cam to true.
Step2.
Upload this example to your mcu.
Step3.
Connect myTransferPortSerialRxPin to the recievers myTransferPortSerialTxPin.
Connect myTransferPortSerialTxPin to the recievers myTransferPortSerialRxPin.

Tested on Pro mini 3.3V, NodeMcu 8266, Esp32 Wroom and ESP32-CAM (AI-Thinker).

compileFor328P uses softwareSerial for transfer the data, hardwareSerial UART 0 for debugging.
compileForEsp8266 uses softwareSerial for transfer the data, hardwareSerial UART 0 for debugging.
compileForEsp32 uses hardwareSerial UART 2 for transfer the data, hardwareSerial UART 0 for debugging.
compileForEsp32Cam uses hardwareSerial UART 2 for transfer the data, hardwareSerial UART 0 for debugging.
*/
//IMPORTANT NOTE:The SoftwareSerial buffer can hold up to 64 bytes.
//NOTE:SoftwareSerial is half duplex, meaning that it can not send and recieve at the same time.

//ONLY set 1 of the below to TRUE.
#define compileFor328P true//Arduino Uno, Nano, pro mini and Mega.
#define compileForEsp8266 false//ESP8266.
#define compileForEsp32 false//ESP WROOM-32.
#define compileForEsp32Cam false//AI Thinker ESP32-CAM.
//ONLY set 1 of the above to TRUE.

#if !compileFor328P && !compileForEsp8266 && !compileForEsp32 && !compileForEsp32Cam
  #error Select a compile option!
#endif

#define myDebugPortBaudRate 115200//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//IMPORTANT:myTransferPortBaudRate must be same on recieve_chunks_with_ack
#define myTransferPortBaudRate 38400//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//NOTE:Not advisable to go higher than 38400 on softwareSerial.

#if compileFor328P
  #define myTransferPortSerialRxPin 9//Recieve pin.
  #define myTransferPortSerialTxPin 8//Transmit pin.
  #include <SoftwareSerial.h>
  SoftwareSerial myTransferPort(myTransferPortSerialRxPin, myTransferPortSerialTxPin);// RX, TX //Create software serial object.
#elif compileForEsp8266
  //https://github.com/esp8266/Arduino/issues/584
  #define myTransferPortSerialRxPin D7//GPIO13 = D7 Recieve pin.
  #define myTransferPortSerialTxPin D6//GPIO12 = D6 Transmit pin.
  #include <SoftwareSerial.h>
  //NOTE:Above should select EspSoftwareSerial (if installed) when selecting ESP8266 boards, but it seems not to do so if you use an older version from boards manager? But still works ok.
  SoftwareSerial myTransferPort(myTransferPortSerialRxPin, myTransferPortSerialTxPin);// RX, TX //Create software serial object.
#elif compileForEsp32
  //We use the second hardware port on ESP32, not software serial.
  #define myTransferPortSerialRxPin 16//GPIO16 UART 2 Recieve pin.
  #define myTransferPortSerialTxPin 17//GPIO17 UART 2 Transmit pin.
#elif compileForEsp32Cam
  //We use the second hardware port on ESP32-CAM, not software serial.
  #include <HardwareSerial.h>
  HardwareSerial myTransferPort(2);//UART2 Serial2 //Create hardware serial object.
  #define myTransferPortSerialRxPin 15//GPIO15 UART 2 Recieve pin.
  #define myTransferPortSerialTxPin 13//GPIO13 UART 2 Transmit pin.
#endif

#define simulateDeveloperErrors false//Set to true to simulate developer errors.

#include "lorem_ipsum.h"//The data we want to send in chunks. //3467 bytes.

#include <DigiHz_EasyTransfer.h>//Include the library.

DigiHz_EasyTransfer myTransfer(true, 1);//Create object with debug and rx buffer size of min 1 byte.
//DigiHz_EasyTransfer myTransfer(true);//Create object with debug and rx buffer default size 254 bytes.

bool chunksSendingStarted = false;//Init flag.
uint8_t currentChunkId = 0;//Init counter.

unsigned long txChunksStartMeasure;//For measure time it takes to send all chunks.
unsigned long txChunksEndMeasure;//For measure time it takes to send all chunks.

const uint16_t chunksSendingWaitTime = 5000;//Wait time between sending the chunks. (Value in milli seconds)
unsigned long previousMillisChunkSendingStopped = 0;//Init //Store when we last stopped sending our chunk data packets.

//NOTE:The value off txChunkWaitTimeBetweenChunks you have to time for yourself in your own final code.
const uint8_t txChunkWaitTimeBetweenChunks = 40;//Time we wait between each chunk we send. (Value in milli seconds).
unsigned long previousMillisSentChunkData = 0;//Init //Store when we last sent our chunk data packet.

#if simulateDeveloperErrors
  //This will show how using to big data to send fails.
  const uint8_t chunksMaxBytes = 249;//Max number of bytes we send at a time. (249 + 6 = 255 bytes) 
#else
  const uint8_t chunksMaxBytes = 128;//Max number of bytes we send at a time. (128 + 6 = 134 bytes)
  //const uint8_t chunksMaxBytes = 210;//Max number of bytes we send at a time. (210 + 6 = 216 bytes)
#endif

uint16_t chunksTotalBytes = strlen(charArrayProgmem);//The total number of bytes we are going to send.
uint8_t numChunks;//Will be calculated in setup.
uint8_t chunksRemainerBytes;//Will be calculated in setup.

//Data packets we send start.
struct SEND_CHUNKS_DATA_STRUCTURE {//Data we send. 5 bytes chunk meta data + chunksMaxBytes bytes chunk data + null byte.
  uint8_t numChunks;//(1 byte) //Will have the total number of chunks in it.
  uint16_t chunksTotalBytes;//(2 bytes) //Will have the total number of bytes for all the chunks in it.
  uint8_t chunkId;//(1 byte) //Will have the current chunk id in it.
  uint8_t chunkActualBytes;//(1 byte) //Will have the size in bytes for the current chunk in it.
  char chunkData[chunksMaxBytes + 1];//Will have the chunk data in it. (chunksMaxBytes + null byte)
//};//Only using this may not make it work with mixed boards.
} __attribute((__packed__)) packet_chunks_send;//This fixes problem with struct size difference between mixed boards. https://github.com/esp8266/Arduino/issues/1825
SEND_CHUNKS_DATA_STRUCTURE txChunkData;//No specific default values for the struct. //Use this if you don't care about the initial values in the struct.
//Data packets we send end.

bool sendChunk(uint8_t chunkId){//Function for sending chunks one by one.
  
  //Populate struct start.
  txChunkData.chunkId = chunkId + 1;//Increase txChunkData.chunkId.
    
  if (chunkId + 1 == numChunks){//If last chunk (or first and only chunk).
    if (chunksRemainerBytes > 0){//If we have remainer.
      txChunkData.chunkActualBytes = chunksRemainerBytes;
    } else {//If we not have remainer.
      txChunkData.chunkActualBytes = chunksMaxBytes;
    }
  } else {//Not last chunk (and not first and only chunk).
    txChunkData.chunkActualBytes = chunksMaxBytes;
  }
  //Populate struct end.

  strncpy_P(txChunkData.chunkData, &charArrayProgmem[chunksMaxBytes * chunkId], chunksMaxBytes);//Copy chunksMaxBytes characters (or less) to txChunkData.chunkData from progmem.
  Serial.print(F("Sending chunk "));Serial.print(chunkId + 1);Serial.print(F(" of "));Serial.println(numChunks);
  Serial.print(F("Chunk data:"));Serial.println(txChunkData.chunkData);
  
  if (myTransfer.sendData(details(txChunkData), 6, true)){//Send struct txChunkData as identifier 6 and wait for ack.
    Serial.print(F("Chunk packet "));Serial.print(chunkId + 1);Serial.println(F(" sent and ack recieved."));
    currentChunkId++;//Increase counter.
    if (currentChunkId == numChunks){//If we sent all chunks.
      txChunksEndMeasure = millis();
      Serial.println();
      Serial.println(F("************************************************"));
      Serial.println(F("Sent the last chunk. (All chunks sent properly.)"));
      Serial.println(F("************************************************"));
      Serial.print(F("Chunk data total bytes sent:"));Serial.print(chunksTotalBytes);Serial.println(F(" bytes."));
      Serial.print(F("Time to send all chunks:"));Serial.print(txChunksEndMeasure - txChunksStartMeasure);Serial.println(F(" milli seconds"));
      Serial.println();
      currentChunkId = 0;//Reset counter.
      Serial.print(F("Waiting "));Serial.print(chunksSendingWaitTime);Serial.println(F(" milli seconds."));
      Serial.println();
      chunksSendingStarted = false;//Set flag.
      previousMillisChunkSendingStopped = millis();//Set time when we sent all chunks.
    }
    return true;
  } else {
    if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
      Serial.println();
      Serial.print(F("ERROR:Chunk data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
      Serial.println();
    } else if (myTransfer.getTxStatus() == myTransfer.TX_INVALID_IDENTIFIER){
      Serial.println();
      Serial.println(F("ERROR:Invalid identifier, identifier 0 is reserved for ack! Packet not sent!"));
      Serial.println();
    } else if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_ACK_TIMEOUT){//NOTE:This will only be returned if ack is set to true.
      Serial.println();
      Serial.println(F("ERROR:Did not get the ack back within the timeout value!"));
      Serial.println();
    }
    return false;
  }
}

void configureMyTransfer(){//In here you have some configuration options avalible for the DigiHz_EasyTransfer library.
  //IMPORTANT:Options MUST be set AFTER begin method.
  //NOTE:You can set any of these options at runtime to if needed.

  //The headers can be changed, but most likely you dont have to.
  //myTransfer.setHeaders();//Default is 0x06, 0x85
  //myTransfer.setHeaders(0x06, 0x85);//Default is 0x06, 0x85
  //myTransfer.setHeaders(0x06);//Default is 0x06, 0x85

  //The number of times a packet is trying to send to the recieving unit. (max value is 255)
  //myTransfer.setTxRetries();//Only have affect if ack is requested in sendData method. //Default is 5
  //myTransfer.setTxRetries(10);//Only have affect if ack is requested in sendData method. //Default is 5

  //The amount of time we wait for the ack packet to get back from the recieving unit. (max value is 65535)
  //myTransfer.setTxAckTimeout();//Sets tx ack timeout to 50 milli seconds. //Only have affect if ack is requested in sendData method. //Default is 50
  //myTransfer.setTxAckTimeout(200);//Sets tx ack timeout to 200 miilli seconds. //Only have affect if ack is requested in sendData method. //Default is 50

  //If you want to clear the temporary rx buffer with zeros on each recieve or not, but most likely you dont have to.
  //myTransfer.clearRxBufferOnRecieve();//Does not clear the temporary rx buffer with zeros on each recieve. //Default is false.
  //myTransfer.clearRxBufferOnRecieve(false);//Does not clear the temporary rx buffer with zeros on each recieve. //Default is false.
  //myTransfer.clearRxBufferOnRecieve(true);//Clears the temporary rx buffer with zeros on each recieve. //Default is false.

  //If you want to flush the rx port buffer or not. (max value is 255)
  //myTransfer.setFlushRxPortBufferWaitTime();//No flush. //Default is 0 milli seconds.
  //myTransfer.setFlushRxPortBufferWaitTime(0);//No flush. //Default is 0 milli seconds.
  //myTransfer.setFlushRxPortBufferWaitTime(20);//Flushes the rx port buffer for 20 mS or until the rx port buffer is empty so we not get garbage when we recieve a data packet. //Default is 0 milli seconds.
}

void setup() {
  // put your setup code here, to run once:
  #if compileFor328P
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recieve_chunks_with_ack.
  #elif compileForEsp8266
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recieve_chunks_with_ack.
  #elif compileForEsp32
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recieve_chunks_with_ack.
  #elif compileForEsp32Cam
    myTransferPort.begin(myTransferPortBaudRate, SERIAL_8N1, myTransferPortSerialRxPin, myTransferPortSerialTxPin);//Must be same baud rate on recieve_chunks_with_ack.
  #endif
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();

  //Calculate numChunks and chunksRemainerBytes start.
  if (chunksTotalBytes % chunksMaxBytes == 0){//No remainer.
    numChunks = chunksTotalBytes / chunksMaxBytes;
    chunksRemainerBytes = 0;
  } else {//Remainer.
    numChunks = (chunksTotalBytes / chunksMaxBytes) + 1;
    chunksRemainerBytes = chunksTotalBytes % chunksMaxBytes;
  }
  //Calculate numChunks and chunksRemainerBytes end.
  
  //Populate struct start.
  txChunkData.numChunks = numChunks;
  txChunkData.chunksTotalBytes = chunksTotalBytes;
  //Populate struct end.

  Serial.print(F("Struct txChunkData occupies "));Serial.print(sizeof(txChunkData));Serial.println(F(" bytes."));
  
  Serial.print(F("chunksMaxBytes:"));Serial.println(chunksMaxBytes);
  Serial.print(F("chunksTotalBytes:"));Serial.print(chunksTotalBytes);Serial.println(F(" bytes."));
  Serial.print(F("numChunks:"));Serial.println(numChunks);
  Serial.print(F("chunksRemainerBytes:"));Serial.print(chunksRemainerBytes);Serial.println(F(" bytes."));
  Serial.println();
  Serial.print(F("Start sending chunks in "));Serial.print(chunksSendingWaitTime);Serial.println(F(" milli seconds."));
  Serial.println();
  
  previousMillisChunkSendingStopped = millis();//Set new time.
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (chunksSendingStarted){//If we started the sending of the chunks.
    if (millis() - previousMillisSentChunkData >= txChunkWaitTimeBetweenChunks) {//Check if it is time to send chunk data.
      if (!sendChunk(currentChunkId)){//Failure sending chunk.
        chunksSendingStarted = false;//Set flag.
        currentChunkId = 0;//Reset counter.
        previousMillisChunkSendingStopped = millis();//Set new time.
        Serial.println(F("Stopped sending chunks because of transmittion failure!"));
        Serial.println(F("Maybe you should increase the txChunkWaitTimeBetweenChunks value?"));
        Serial.println(F("Or decrease the value of chunksMaxBytes?"));
        Serial.println();
      }
      previousMillisSentChunkData = millis();
    }
  } else {//If we not started to send chunks.
    if (millis() - previousMillisChunkSendingStopped >= chunksSendingWaitTime) {//Check if it is time to start sending chunk data.
      Serial.println(F("Starting sending chunks..."));
      Serial.println();
      chunksSendingStarted = true;//Set flag.
      txChunksStartMeasure = millis();
    }
  }
}
