/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example recieves data with ack from the sending unit (send_with_ack).

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
//IMPORTANT:myTransferPortBaudRate must be same on send_with_ack
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
  #define myTransferPortSerialRxPin 15//GPIO15 Recieve pin.
  #define myTransferPortSerialTxPin 13//GPIO13 Transmit pin.
#endif

#include "parsing_helpers.h"//Helper functions for parsing recieved data.

#include <DigiHz_EasyTransfer.h>

DigiHz_EasyTransfer myTransfer(true);//Create object with debug and rx buffer default size 254 bytes.
//DigiHz_EasyTransfer myTransfer(true, 64);//Create object with debug and rx buffer size 64 bytes.

unsigned long checkLongLength;//A var we use for checking if a long is 4 or 8 bytes on this mcu.

//NOTE:You can parse the recived struct in 2 different ways. (The result is the same)
//Set parseRecievedStructWithStruct to false if you want to parse the struct with parsing helpers. //This is more complex to calculate.
//Set parseRecievedStructWithStruct to true if you want to parse the struct with memcpy and struct. //This is the easiest way.
#define parseRecievedStructWithStruct true

#if parseRecievedStructWithStruct
  struct RECIEVED_TEST_DATA_STRUCTURE {//IMPORTANT:Must be same structure as in send_with_ack.
    bool myBool;//(1 byte)
    char myChar;//(1 byte)
    char myCharArray[12];//(11 bytes + nullbyte)
    float myFloatPos;//(4 bytes)
    float myFloatNeg;//(4 bytes)
    uint8_t myUint8_tMax;//(1 byte)
    int8_t myInt8_tMin;//(1 byte)
    int8_t myInt8_tMax;//(1 byte)
    uint16_t myUint16_tMax;//(2 bytes)
    int16_t myInt16_tMin;//(2 bytes)
    int16_t myInt16_tMax;//(2 bytes)
    int16_t myInt16_tArrayPos[5];//(2 bytes * 5 = 10 bytes)
    int16_t myInt16_tArrayNeg[5];//(2 bytes * 5 = 10 bytes)
    uint32_t myUint32_tMax;//(4 bytes)
    int32_t myInt32_tMin;//(4 bytes)
    int32_t myInt32_tMax;//(4 bytes)
    uint64_t myUint64_tMax;//(8 bytes)
    int64_t myInt64_tMin;//(8 bytes)
    int64_t myInt64_tMax;//(8 bytes)
    /*
    //IMPORTANT:
    Since a long is different in size (4 bytes or 8 bytes depending on architechture)
    We do not use the long variable in this example, because we dont know what you are running on.
    But if you know what you are doing then you can use long to of course.
    */
    /*
    unsigned long myUnsignedLong_Max;//(4 bytes or 8 bytes depending on architechture)
    long myLong_Min;//(4 bytes or 8 bytes depending on architechture)
    long myLong_Max;//(4 bytes or 8 bytes depending on architechture)
    */
  //};//Only using this may not make it work with mixed boards.
  } __attribute((__packed__)) packet_test_recieved;//This fixes problem with struct size difference between mixed boards. https://github.com/esp8266/Arduino/issues/1825
  RECIEVED_TEST_DATA_STRUCTURE rxTestData;//No specific default values for the struct. //Use this if you don't care about the initial values in the struct.
#endif

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
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_with_ack.
  #elif compileForEsp8266
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_with_ack.
  #elif compileForEsp32
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on send_with_ack.
  #elif compileForEsp32Cam
    myTransferPort.begin(myTransferPortBaudRate, SERIAL_8N1, myTransferPortSerialRxPin, myTransferPortSerialTxPin);//Must be same baud rate on send_with_ack.
  #endif
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();
  Serial.print(F("A long is "));Serial.print(sizeof(checkLongLength));Serial.println(F(" bytes on this mcu."));
  #if parseRecievedStructWithStruct
    Serial.print(F("Struct rxTestData occupies "));Serial.print(sizeof(rxTestData));Serial.println(F(" bytes."));
  #endif
  Serial.println();
  delay(2000);//Give some time to read the output.
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
    
    if (myTransfer.rxIdentifier == 1){//testByte (1 byte)
      Serial.print(F("testByte:"));Serial.println(myTransfer.rxBuffer[0]);//Byte 1.
    } else if (myTransfer.rxIdentifier == 2){//testFloat (4 bytes)
      Serial.print(F("testFloat:"));Serial.println(processBytes2Float(myTransfer.rxBuffer, 0), 6);//Byte 1, 2, 3 and 4. //Prints out with 6 decimals.
    } else if (myTransfer.rxIdentifier == 3){//testUint8_tArray (10 bytes)
      for (int i = 0; i < myTransfer.rxBufferLength; i++){//Byte 1 to 10.
        Serial.print(F("testUint8_tArray["));Serial.print(i);Serial.print(F("]:"));Serial.println(myTransfer.rxBuffer[i]);
      }
    } else if (myTransfer.rxIdentifier == 4){//testCharArray (12 bytes)
      Serial.print(F("testCharArray:"));
      for (int i = 0; i < myTransfer.rxBufferLength; i++){//byte 1 to 11 + the nullbyte.
        Serial.print((char)myTransfer.rxBuffer[i]);
      }
      Serial.println();
    } else if (myTransfer.rxIdentifier == 5){//txTestData
      #if !parseRecievedStructWithStruct
        Serial.println();
        Serial.println(F("Struct parsed with parsing helpers."));
        if (myTransfer.rxBuffer[0]){//myBool (1 byte) //Byte 1.
          Serial.println(F("myBool:true"));
        } else {
          Serial.println(F("myBool:false"));
        }
        Serial.print(F("myChar:"));Serial.println((char)myTransfer.rxBuffer[1]);//myChar (1 byte) //Byte 2.
        Serial.print(F("myCharArray:"));
        for (int i = 0; i < 11; i++){//myCharArray (11 bytes) //Byte 3 to 13.
          Serial.print((char)myTransfer.rxBuffer[i + 2]);
        }
        Serial.println();
        //NOTE:We skip byte 14 because it is a nullbyte.
        Serial.print(F("myFloatPos:"));Serial.println(processBytes2Float(myTransfer.rxBuffer, 14), 6);//myFloatPos (pi) (4 bytes) //Byte 15, 16, 17 and 18. //Prints out with 6 decimals.
        Serial.print(F("myFloatNeg:"));Serial.println(processBytes2Float(myTransfer.rxBuffer, 18), 6);//myFloatNeg (pi) (4 bytes) //Byte 19, 20, 21 and 22. //Prints out with 6 decimals.
        Serial.print(F("myUint8_tMax:"));Serial.println(myTransfer.rxBuffer[22]);//myUint8_tMax (1 byte) //Byte 23.
        Serial.print(F("myInt8_tMin:"));Serial.println(processByte2Int8_t(myTransfer.rxBuffer, 23));//myInt8_tMin (1 byte) //Byte 24.
        Serial.print(F("myInt8_tMax:"));Serial.println(processByte2Int8_t(myTransfer.rxBuffer, 24));//myInt8_tMax (1 byte) //Byte 25.
        Serial.print(F("myUint16_tMax:"));Serial.println(processBytes2Uint16_t(myTransfer.rxBuffer, 25));//myUint16_tMax (2 bytes) //Byte 26 and 27.
        Serial.print(F("myInt16_tMin:"));Serial.println(processBytes2Int16_t(myTransfer.rxBuffer, 27));//myInt16_tMin (2 bytes) //Byte 28 and 29.
        Serial.print(F("myInt16_tMax:"));Serial.println(processBytes2Int16_t(myTransfer.rxBuffer, 29));//myInt16_tMax (2 bytes) //Byte 30 and 31.
        for (int i = 0; i < 5; i++){//Byte 32 to 41
          Serial.print(F("myInt16_tArrayPos["));Serial.print(i);Serial.print(F("]:"));Serial.println(processBytes2Int16_t(myTransfer.rxBuffer, 31 + (i * 2)));//myInt16_tArrayPos (2 bytes * 5 = 10 bytes)
        }
        for (int i = 0; i < 5; i++){//Byte 42 to 51
          Serial.print(F("myInt16_tArrayNeg["));Serial.print(i);Serial.print(F("]:"));Serial.println(processBytes2Int16_t(myTransfer.rxBuffer, 41 + (i * 2)));//myInt16_tArrayNeg (2 bytes * 5 = 10 bytes)
        }
        Serial.print(F("myUint32_tMax:"));Serial.println(processBytes2Uint32_t(myTransfer.rxBuffer, 51));//myUint32_tMax (4 bytes) //Byte 52, 53, 54 and 55.
        Serial.print(F("myInt32_tMin:"));Serial.println(processBytes2Int32_t(myTransfer.rxBuffer, 55));//myInt32_tMin (4 bytes) //Byte 56, 57, 58 and 59.
        Serial.print(F("myInt32_tMax:"));Serial.println(processBytes2Int32_t(myTransfer.rxBuffer, 59));//myInt32_tMax (4 bytes) //Byte 60, 61, 62 and 63.
        Serial.print(F("myUint64_tMax:"));printlnUint64_t(&Serial, processBytes2Uint64_t(myTransfer.rxBuffer, 63));//myUint64_tMax (8 bytes) //Byte 64, 65, 66, 67, 68, 69, 70 and 71.  
        Serial.print(F("myInt64_tMin:"));printlnInt64_t(&Serial, processBytes2Int64_t(myTransfer.rxBuffer, 71));//myInt64_tMin (8 bytes) //Byte 72, 73, 74, 75, 76, 77, 78 and 79.
        Serial.print(F("myInt64_tMax:"));printlnInt64_t(&Serial, processBytes2Int64_t(myTransfer.rxBuffer, 79));//myInt64_tMax (8 bytes) //Byte 80, 81, 82, 83, 84, 85, 86 and 87.
        /*
        //IMPORTANT:
        Since a long is different in size (4 bytes or 8 bytes depending on architechture)
        We do not use the long variable in this example, because we dont know what you are running on.
        But if you know what you are doing then you can use long to of course.
        */
        /*
        if (sizeof(checkLongLength = 4)){//Check if a long is 4 or 8 bytes long on this mcu.
          //long 4 bytes start.
          Serial.println(F("long is 4 bytes on this mcu."));
          Serial.print(F("myUnsignedLong_Max:"));Serial.println(processBytes2UnsignedLong(myTransfer.rxBuffer, 87));//myUnsignedLong_Max (4 bytes) //Byte 88, 89, 90 and 91.
          Serial.print(F("myLong_Min:"));Serial.println(processBytes2Long(myTransfer.rxBuffer, 91));//myLong_Min (4 bytes) //Byte 92, 93, 94 and 95.
          Serial.print(F("myLong_Max:"));Serial.println(processBytes2Long(myTransfer.rxBuffer, 95));//myLong_Max (4 bytes) //Byte 96, 97, 98 and 99.
          //long 4 bytes end.
        } else {
          //long 8 bytes start.
          Serial.println(F("long is 8 bytes on this mcu."));
          Serial.print(F("myUnsignedLong_Max:"));printlnUint64_t(&Serial, processBytes2UnsignedLong(myTransfer.rxBuffer, 87, true));//myUnsignedLong_Max (8 bytes) //Byte 88, 89, 90, 91, 92, 93, 94 and 95.  
          Serial.print(F("myLong_Min:"));printlnInt64_t(&Serial, processBytes2Long(myTransfer.rxBuffer, 95, true));//myLong_Min (8 bytes) //Byte 96, 97, 98, 99, 100, 101, 102 and 103.  
          Serial.print(F("myLong_Max:"));printlnInt64_t(&Serial, processBytes2Long(myTransfer.rxBuffer, 103, true));//myLong_Max (8 bytes) //Byte 104, 105, 106, 107, 108, 109, 110 and 111.  
          //long 8 bytes end.
        }
        */
      #else
        memcpy(&rxTestData, myTransfer.rxBuffer, myTransfer.rxBufferLength);//Copy our recieved struct from rxBuffer to rxTestData struct.
        Serial.println();
        Serial.println(F("Struct parsed with memcpy to rxTestData struct."));
        if (rxTestData.myBool){//myBool (1 byte)
          Serial.println(F("myBool:true"));
        } else {
          Serial.println(F("myBool:false"));
        }
        Serial.print(F("myChar:"));Serial.println(rxTestData.myChar);//myChar (1 byte)
        Serial.print(F("myCharArray:"));Serial.println(rxTestData.myCharArray);//myCharArray (11 byte + nullbyte) 
        Serial.print(F("myFloatPos:"));Serial.println(rxTestData.myFloatPos, 6);//myFloatPos (4 bytes) //Prints out with 6 decimals.
        Serial.print(F("myFloatNeg:"));Serial.println(rxTestData.myFloatNeg, 6);//myFloatNeg (4 bytes) //Prints out with 6 decimals.
        Serial.print(F("myUint8_tMax:"));Serial.println(rxTestData.myUint8_tMax);//myUint8_tMax (1 byte)
        Serial.print(F("myInt8_tMin:"));Serial.println(rxTestData.myInt8_tMin);//myInt8_tMin (1 byte)
        Serial.print(F("myInt8_tMax:"));Serial.println(rxTestData.myInt8_tMax);//myInt8_tMax (1 byte)
        Serial.print(F("myUint16_tMax:"));Serial.println(rxTestData.myUint16_tMax);//myUint16_tMax (2 bytes)
        Serial.print(F("myInt16_tMin:"));Serial.println(rxTestData.myInt16_tMin);//myInt16_tMin (2 bytes)
        Serial.print(F("myInt16_tMax:"));Serial.println(rxTestData.myInt16_tMax);//myInt16_tMax (2 bytes)
        for (int i = 0; i < 5; i++){//5 values in array myInt16_tArrayPos
          Serial.print(F("myInt16_tArrayPos["));Serial.print(i);Serial.print(F("]:"));Serial.println(rxTestData.myInt16_tArrayPos[i]);//myInt16_tArrayPos (2 bytes * 5 = 10 bytes)
        }
        for (int i = 0; i < 5; i++){//5 values in array myInt16_tArrayNeg
          Serial.print(F("myInt16_tArrayNeg["));Serial.print(i);Serial.print(F("]:"));Serial.println(rxTestData.myInt16_tArrayNeg[i]);//myInt16_tArrayNeg (2 bytes * 5 = 10 bytes)
        }
        Serial.print(F("myUint32_tMax:"));Serial.println(rxTestData.myUint32_tMax);//myUint32_tMax (4 bytes)
        Serial.print(F("myInt32_tMin:"));Serial.println(rxTestData.myInt32_tMin);//myInt32_tMin (4 bytes)
        Serial.print(F("myInt32_tMax:"));Serial.println(rxTestData.myInt32_tMax);//myInt32_tMax (4 bytes)
        Serial.print(F("myUint64_tMax:"));printlnUint64_t(&Serial, rxTestData.myUint64_tMax);//myUint64_tMax (8 bytes)
        Serial.print(F("myInt64_tMin:"));printlnInt64_t(&Serial, rxTestData.myInt64_tMin);//myInt64_tMin (8 bytes)
        Serial.print(F("myInt64_tMax:"));printlnInt64_t(&Serial, rxTestData.myInt64_tMax);//myInt64_tMax (8 bytes)
        /*
        //IMPORTANT:
        Since a long is different in size (4 bytes or 8 bytes depending on architechture)
        We do not use the long variable in this example, because we dont know what you are running on.
        But if you know what you are doing then you can use long to of course.
        */
        /*
        if (sizeof(checkLongLength = 4)){//Check if a long is 4 or 8 bytes long on this mcu.
          //long 4 bytes start.
          Serial.println(F("long is 4 bytes on this mcu."));
          Serial.print(F("myUnsignedLong_Max:"));Serial.println(rxTestData.myUnsignedLong_Max);//myUnsignedLong_Max (4 bytes)
          Serial.print(F("myLong_Min:"));Serial.println(rxTestData.myLong_Min);//myLong_Min (4 bytes)
          Serial.print(F("myLong_Max:"));Serial.println(rxTestData.myLong_Max);//myLong_Max (4 bytes)
          //long 4 bytes end.
        } else {
          //long 8 bytes start.
          Serial.println(F("long is 8 bytes on this mcu."));
          Serial.print(F("myUnsignedLong_Max:"));printlnUint64_t(&Serial, rxTestData.myUnsignedLong_Max);//myUnsignedLong_Max (8 bytes)
          Serial.print(F("myLong_Min:"));printlnInt64_t(&Serial, rxTestData.myLong_Min);//myLong_Min (8 bytes)
          Serial.print(F("myLong_Max:"));printlnInt64_t(&Serial, rxTestData.myLong_Max);//myLong_Max (8 bytes)
          //long 8 bytes end.
        }
        */
      #endif
    } else if (myTransfer.rxIdentifier == 7){//testCharArray2
      Serial.print(F("testCharArray2:"));
      for (int i = 0; i < myTransfer.rxBufferLength; i++){//byte 1 to myTransfer.rxBufferLength.
        Serial.print((char)myTransfer.rxBuffer[i]);
      }
      Serial.println();
    } else {
      Serial.println(F("Recieved unknown identifier!"));
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
}
