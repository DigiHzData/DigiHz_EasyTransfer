/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example recieves chunk data with ack from the sending unit (send_chunks_with_ack).

Step1.
Set compileFor328P OR compileForEsp8266 OR compileForEsp32 OR compileForEsp32Cam to true.
Step2.
Upload this example to your mcu.
Step3.
Connect myTransferPortSerialRxPin to the senders myTransferPortSerialTxPin.
Connect myTransferPortSerialTxPin to the senders myTransferPortSerialRxPin.

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
//IMPORTANT:myTransferPortBaudRate must be same on send_chunks_with_ack
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
  #define myTransferPort Serial2
#elif compileForEsp32Cam
  //We use the second hardware port on ESP32-CAM, not software serial.
  #include <HardwareSerial.h>
  HardwareSerial myTransferPort(2);//UART2 Serial2 //Create hardware serial object.
  #define myTransferPortSerialRxPin 15//GPIO15 Recieve pin.
  #define myTransferPortSerialTxPin 13//GPIO13 Transmit pin.
#endif

#include "parsing_helpers.h"//Helper functions for parsing recieved data.

#include <DigiHz_EasyTransfer.h>//Include the library.

DigiHz_EasyTransfer myTransfer(true);//Create object with debug and rx buffer default size 254 bytes.
//DigiHz_EasyTransfer myTransfer(true, 134);//Create object with debug and rx buffer size 134 bytes.

bool chunksRecieveStarted = false;//Init flag.
uint8_t currentChunkIdRecieved;//The current chunkId we recieve.
uint8_t previousChunkIdRecieved;//For catching duplicate reads from the rx buffer.
unsigned long firstChunkRecieved;//Will store when the first chunk was recieved.

//NOTE:The value off timeoutRecieveNextChunk you have to time for yourself in your own final code.
const uint16_t timeoutRecieveNextChunk = 100;//How long time we wait maximum for next chunk to be recieved. (Value in milli seconds).
unsigned long previousChunkRecieved;//Will store when a chunk was recieved.

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
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_chunks_with_ack.
  #elif compileForEsp8266
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_chunks_with_ack.
  #elif compileForEsp32
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_chunks_with_ack.
  #elif compileForEsp32Cam
    myTransferPort.begin(myTransferPortBaudRate, SERIAL_8N1, myTransferPortSerialRxPin, myTransferPortSerialTxPin);//Must be same baud rate on send_chunks_with_ack.
  #endif
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (myTransfer.receiveData()){//If we recieved anything valid.
    Serial.println();
    Serial.print(F("Recieved identifier "));Serial.println(myTransfer.rxIdentifier);
    Serial.print(F("Recieved bytes "));Serial.println(myTransfer.rxBufferLength);
    
    /*
    //myTransfer.rxBuffer[0] to myTransfer.rxBuffer[myTransfer.rxBufferLength] is the data we recieved.
    Serial.println();Serial.println(F("rx buffer start."));
    for (uint8_t i = 0; i < myTransfer.rxBufferLength; i++){
      Serial.print(F("byte "));Serial.print(i + 1);Serial.print(F(" = "));Serial.println(myTransfer.rxBuffer[i]);
    }
    Serial.println(F("rx buffer end."));Serial.println();
    */
    
    if (myTransfer.rxIdentifier == 6){//txChunkData
      if (myTransfer.rxBuffer[3] == 1){//If it is the first chunk. //chunkId (1 byte) //Byte 4.
        firstChunkRecieved = millis();//Store when the first chunk was recieved.
        previousChunkRecieved = millis();//Store when a chunk was recieved.
        if (!chunksRecieveStarted){
          chunksRecieveStarted = true;//Set flag.
          currentChunkIdRecieved = myTransfer.rxBuffer[3];//Set value. //chunkId (1 byte) //Byte 4.
          previousChunkIdRecieved = 0;//Set value.
          Serial.println(F("Struct parsed with parsing helpers."));
          Serial.print(F("chunkId:"));Serial.println(myTransfer.rxBuffer[3]);//chunkId (1 byte) //Byte 4.
          Serial.println(F("Got the first chunk"));
          Serial.print(F("chunk data bytes recieved:"));Serial.print(myTransfer.rxBuffer[4]);Serial.println(F(" bytes."));//chunkActualBytes (1 byte) //Byte 5.
          Serial.print(F("Chunk data recieved:"));
          for (int i = 5; i < myTransfer.rxBufferLength; i++){//chunkData //Byte 6 to myTransfer.rxBufferLength.
            Serial.print((char)myTransfer.rxBuffer[i]);
          }
          Serial.println();
        } else {//Duplicate read of rx buffer.
          previousChunkRecieved = millis();//Update timeout time.
          Serial.print(F("chunkId:"));Serial.println(myTransfer.rxBuffer[3]);//chunkId (1 byte) //Byte 4.
          Serial.println(F("Duplicate read of first chunk (ignoring it.)"));
        }
      } else {//Not first chunk.
        if (chunksRecieveStarted){//If we recieved the first chunk.
          previousChunkIdRecieved++;//increase value by 1.
          currentChunkIdRecieved = myTransfer.rxBuffer[3];//Set value. //chunkId (1 byte) //Byte 4.
          if (currentChunkIdRecieved - 1 == previousChunkIdRecieved){//If it is the next chunk we read.
            previousChunkRecieved = millis();//Store when a chunk was recieved.
            Serial.print(F("chunkId:"));Serial.println(myTransfer.rxBuffer[3]);//chunkId (1 byte) //Byte 4.
            Serial.print(F("chunk data bytes recieved:"));Serial.print(myTransfer.rxBuffer[4]);Serial.println(F(" bytes."));//chunkActualBytes (1 byte) //Byte 5.
            Serial.print(F("Chunk data recieved:"));
            for (int i = 5; i < myTransfer.rxBufferLength; i++){//chunkData //Byte 6 to myTransfer.rxBufferLength.
              Serial.print((char)myTransfer.rxBuffer[i]);
            }
            Serial.println();
            if (currentChunkIdRecieved == myTransfer.rxBuffer[0]){//Check if we got the last chunk. //numChunks (1 byte) //Byte 1.
              unsigned long lastChunkRecieved = millis();//Store when the last chunk was recieved.
              uint16_t chunkDataRecievedTotalBytes = processBytes2Uint16_t(myTransfer.rxBuffer, 1);//chunksTotalBytes (2 bytes) //Byte 2 and 3.
              Serial.println();
              Serial.println(F("********************************************************"));
              Serial.println(F("Recieved the last chunk. (All chunks recieved properly.)"));
              Serial.println(F("********************************************************"));
              Serial.print(F("Chunk data total bytes recieved:"));Serial.print(chunkDataRecievedTotalBytes);Serial.println(F(" bytes."));
              Serial.print(F("Time to recieve chunks:"));Serial.print(lastChunkRecieved - firstChunkRecieved);Serial.println(F(" milli seconds"));
              chunksRecieveStarted = false;//Reset flag.
            }
          } else if (currentChunkIdRecieved == previousChunkIdRecieved){//If it is a duplicate read of rx buffer.
            previousChunkRecieved = millis();//Update timeout time.
            Serial.print(F("chunkId:"));Serial.println(myTransfer.rxBuffer[3]);//chunkId (1 byte) //Byte 4.
            Serial.println(F("Duplicate read of chunk (ignoring it.)"));
            //Roll back and just ignore this read and continue.
            previousChunkIdRecieved--;//Roll back.  
          }
        }
      }
    } else {
      Serial.print(F("Recieved unknown identifier:"));Serial.println(myTransfer.rxIdentifier);
    }
    Serial.println();
  } else {//If any error.
    if (myTransfer.getRxStatus() == myTransfer.RX_STATUS_BUFFER_TO_SMALL){//NOTE:This error might also show if the rx port buffer is not flushed quickly enough.
      Serial.println(F("ERROR:rx buffer size is set to small! ("));Serial.print(myTransfer.getRxBufferSize());Serial.print(F(" bytes.) Increase the rx buffer size."));
      Serial.println();
    } else if (myTransfer.getRxStatus() == myTransfer.RX_STATUS_DATA_CHECKSUM_FAILED){
      Serial.println(F("ERROR:Data checksum failed!"));
      Serial.println();
    } else if (myTransfer.getRxStatus() == myTransfer.RX_STATUS_ACK_CHECKSUM_FAILED){
      Serial.println(F("ERROR:Ack checksum failed!"));
      Serial.println();
    }
  }

  if (chunksRecieveStarted){//If we are processing the chunks.
    if (millis() - previousChunkRecieved >= timeoutRecieveNextChunk){//Did not catch the next chunk packet in time. (packet lost or skipped).
      //Abort more processing because we did not get a chunk in time.
      chunksRecieveStarted = false;//Set flag.
      Serial.println(F("ERROR:TIMEOUT RECIEVING CHUNK!"));
      Serial.println(F("ERROR:Did not catch the next chunk packet in time. (packet lost or skipped)"));
      Serial.print(F("Skipped after chunkId:"));Serial.println(previousChunkIdRecieved + 1);
      Serial.println(F("Maybe you should increase the timeoutRecieveNextChunk value?"));
      Serial.println();
    }
  }

}
