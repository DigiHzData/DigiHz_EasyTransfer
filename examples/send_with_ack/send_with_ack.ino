/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example sends data with ack to the recieving unit (recive_with_ack).

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
//IMPORTANT:myTransferPortBaudRate must be same on recive_with_ack
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

#define simulateDeveloperErrors false //Set to true to simulate developer errors.

#include <DigiHz_EasyTransfer.h>

DigiHz_EasyTransfer myTransfer(true, 1);//Create object with debug and rx buffer size of min 1 byte.
//DigiHz_EasyTransfer myTransfer(true);//Create object with debug and rx buffer default size 254 bytes.

unsigned long checkLongLength;//A var we use for checking if a long is 4 or 8 bytes on this mcu.
/*
//IMPORTANT:
Since a long is different in size (4 bytes or 8 bytes depending on architechture)
We do not use the long variable in this example, because we dont know what you are running on.
But if you know what you are doing then you can use long to of course.
*/
/*
#define longLength 4//Set to 8 if your mcu reports a length of 8 bytes, else set it to 4.
*/

#define delayTimeBetweenSendingDataTypes 500 //How long time we wait between sending each data type. (Value in milli seconds)
#define intervalSendDataTypes 5000 //How often we want to send our data types. (Value in milli seconds)
unsigned long previousMillisSentDataTypes = 0;//Init //Store when we last sent all our data types.

//Data packets we send start.
byte testByte = 128;//1 byte.
float testFloat = 3.141593;//4 bytes.
uint8_t testUint8_tArray[] = {9,8,7,6,5,4,3,2,1,0};//10 bytes.
char testCharArray[] = {"Hello world"};//11 bytes + nullbyte.

struct SEND_TEST_DATA_STRUCTURE {
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
} __attribute((__packed__)) packet_test_send;//This fixes problem with struct size difference between mixed boards. https://github.com/esp8266/Arduino/issues/1825
//SEND_TEST_DATA_STRUCTURE txTestData;//No specific default values for the struct. //Use this if you don't care about the initial values in the struct.

SEND_TEST_DATA_STRUCTURE txTestData = {//Specific default values for the struct.
  true,//myBool
  'A',//myChar
  {"Hello world"},//myCharArray
  3.141593,//myFloatPos (pi)
  -3.141593,//myFloatNeg (pi)
  255,//myUint8_tMax (max value)
  -128,//myInt8_tMin (min value)
  127,//myInt8_tMax (max value)
  65535,//myUint16_tMax (max value)
  -32768,//myInt16_tMin (min value)
  32767,//myInt16_tMax (max value)
  {5, 10, 15, 20, 25},//myInt16_tArrayPos
  {-10, -20, -30, -40, -50},//myInt16_tArrayNeg
  4294967295,//myUint32_tMax (max value)
  -2147483648,//myInt32_tMin (min value)
  2147483647,//myInt32_tMax (max value)
  18446744073709551615,//myUint64_tMax (max value)
  -9223372036854775808,//myInt64_tMin (min value)
  9223372036854775807,//myInt64_tMax (max value)
  /*
  //IMPORTANT:
  Since a long is different in size (4 bytes or 8 bytes depending on architechture)
  We do not use the long variable in this example, because we dont know what you are running on.
  But if you know what you are doing then you can use long to of course.
  */
  /*
  #if longLength == 4
    //long 4 bytes start.
    4294967295,//myUnsignedLong_Max (max value) (4 bytes depending on architechture)
    -2147483648,//myLong_Min (min value) (4 bytes depending on architechture)
    2147483647//myLong_Max (max value) (4 bytes depending on architechture)
    //long 4 bytes end.
  #else
    //long 8 bytes start.
    18446744073709551615,//myUnsignedLong_Max (max value) (8 bytes depending on architechture)
    -9223372036854775808,//myLong_Min (min value) (8 bytes depending on architechture)
    9223372036854775807//myLong_Max (max value) (8 bytes depending on architechture)
    //long 8 bytes end.
  #endif
  */
};

#if simulateDeveloperErrors
  //This will show how using to big data to send fails.
  //80 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer."};//80 bytes.
  //100 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Numbers:0123456789."};//100 bytes.
  //116 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Letters:abcdefghijklmnopqrstuvwxyz."};//116 bytes.
  //127 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Letters:abcdefghijklmnopqrstuvwxyz. 0123456789"};//127 bytes.
  //135 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Numbers:0123456789. Letters:abcdefghijklmnopqrstuvwxyz"};//135 bytes.
  //199 bytes. Seams to be the max we can send and recive. Why is 199 the limit and not 254 or 255?
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Letters:abcdefghijklmnopqrstuvwxyz. 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF01"};//199 bytes.
  //218 bytes.
  //char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Letters:abcdefghijklmnopqrstuvwxyz. Programming is fun, but can also be very difficult sometimes...Lucky that internet and google exists."};//218 bytes.
  //255 bytes.
  char testCharArray2[] = {"This is a char array of many bytes that we are sending with DigiHz_EasyTransfer. Letters:abcdefghijklmnopqrstuvwxyz. Programming is fun, but can also be very difficult sometimes...Lucky that internet and google exists.0123456789012345678901234567890123456"};//255 bytes. 
#endif
//Data packets we send end.

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
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recive_with_ack.
  #elif compileForEsp8266
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recive_with_ack.
  #elif compileForEsp32
    myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on recive_with_ack.
  #elif compileForEsp32Cam
    myTransferPort.begin(myTransferPortBaudRate, SERIAL_8N1, myTransferPortSerialRxPin, myTransferPortSerialTxPin);//Must be same baud rate on recive_with_ack.
  #endif
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();
  Serial.print(F("A long is "));Serial.print(sizeof(checkLongLength));Serial.println(F(" bytes on this mcu."));
  Serial.print(F("Struct txTestData occupies "));Serial.print(sizeof(txTestData));Serial.println(F(" bytes."));
  Serial.println();
  delay(2000);//Give some time to read the output.
  previousMillisSentDataTypes = millis() + intervalSendDataTypes;//Start sending our data types as soon as possible.
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - previousMillisSentDataTypes >= intervalSendDataTypes) {//Check if it is time to transmit data.
    
    Serial.println(F("Sending var testByte..."));
    if (myTransfer.sendData(details(testByte), 1, true)){//Send var testByte as identifier 1 and wait for ack.
      Serial.println(F("testByte sent and ack recieved."));Serial.println();
    } else {
      if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
        Serial.println();
        Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
    }
    delay(delayTimeBetweenSendingDataTypes);

    Serial.println(F("Sending var testFloat..."));
    if (myTransfer.sendData(details(testFloat), 2, true)){//Send var testFloat as identifier 2 and wait for ack.
      Serial.println(F("testFloat sent and ack recieved."));Serial.println();
    } else {
      if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
        Serial.println();
        Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
    }
    delay(delayTimeBetweenSendingDataTypes);

    Serial.println(F("Sending array testUint8_tArray..."));
    if (myTransfer.sendData(details(testUint8_tArray), 3, true)){//Send array testUint8_tArray as identifier 3 and wait for ack.
      Serial.println(F("testUint8_tArray sent and ack recieved."));Serial.println();
    } else {
      if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
        Serial.println();
        Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
    }
    delay(delayTimeBetweenSendingDataTypes);

    Serial.println(F("Sending array testCharArray..."));
    if (myTransfer.sendData(details(testCharArray), 4, true)){//Send array testCharArray as identifier 4 and wait for ack.
      Serial.println(F("testCharArray sent and ack recieved."));Serial.println();
    } else {
      if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
        Serial.println();
        Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
    }
    delay(delayTimeBetweenSendingDataTypes);

    Serial.println(F("Sending struct txTestData..."));
    if (myTransfer.sendData(details(txTestData), 5, true)){//Send struct txTestData as identifier 5 and wait for ack.
      Serial.println(F("txTestData sent and ack recieved."));Serial.println();
    } else {
      if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
        Serial.println();
        Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
    }
    delay(delayTimeBetweenSendingDataTypes);

    #if simulateDeveloperErrors
      //This will show how using identifier 0 fails.
      Serial.println(F("Sending struct txTestData..."));
      if (myTransfer.sendData(details(txTestData), 0, true)){//Send struct txTestData as identifier 0 and wait for ack.
        Serial.println(F("txTestData sent."));Serial.println();
      } else {
        if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
          Serial.println();
          Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
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
      }
      delay(delayTimeBetweenSendingDataTypes);

      //This will show how using to big data to send fails.
      Serial.println(F("Sending array testCharArray2..."));
      if (myTransfer.sendData(details(testCharArray2), 7, true)){//Send array testCharArray2 as identifier 7 and wait for ack.
        Serial.println(F("testCharArray2 sent."));Serial.println();
      } else {
        if (myTransfer.getTxStatus() == myTransfer.TX_STATUS_DATA_TO_BIG){
          Serial.println();
          Serial.print(F("ERROR:Data packet exceeded the max size of "));Serial.print(myTransfer.txMaxSize);Serial.println(F(" bytes! Packet not sent!"));
          Serial.print(F("size of testCharArray2 is "));Serial.print(sizeof(testCharArray2));Serial.print(F(" bytes."));
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
      }
      delay(delayTimeBetweenSendingDataTypes);
      
    #endif
    
    previousMillisSentDataTypes = millis();
  }
}
