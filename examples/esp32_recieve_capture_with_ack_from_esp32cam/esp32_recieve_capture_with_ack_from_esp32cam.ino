/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example recieves the captured image chunk data with ack from the sending unit (esp32cam_send_capture_with_ack_to_esp32) and sends the chunks via websockets to a browser.
We remarked a lot of serial prints in the sketch so the transfer will go in the supposed speed.
But we kept it there for you to debug easier if you want to.

Step1.
Upload this example to your mcu (ESP32).
Step2.
Connect myTransferPortSerialRxPin to the senders myTransferPortSerialTxPin.
Connect myTransferPortSerialTxPin to the senders myTransferPortSerialRxPin.
Step3.
Connect to access point ESP32-Access-Point and go to:192.168.4.1 in a browser.
*/

#define myDebugPortBaudRate 115200//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//IMPORTANT:myTransferPortBaudRate must be same on esp32cam_send_capture_with_ack_to_esp32
//#define myTransferPortBaudRate 38400//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//#define myTransferPortBaudRate 115200//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
#define myTransferPortBaudRate 230400//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//#define myTransferPortBaudRate 460800//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.

//We use the second hardware port on ESP32, not software serial.
#define myTransferPortSerialRxPin 16//GPIO16 UART 2 Recieve pin.
#define myTransferPortSerialTxPin 17//GPIO17 UART 2 Transmit pin.
#define myTransferPort Serial2

//Wifi start.
#include <WiFi.h>//Standard lib. (Used for wifi)
//Set your access point network credentials.
const char* apSsid = "ESP32-Access-Point";
//If useApPassword is set to false, then no password is needed to connect to the access point.
//If useApPassword is set to true, then a password is needed to connect to the access point.
#define useApPassword false
#if useApPassword
  const char* apPassword = "12345678";
#endif
//Wifi end.

//Webserver start.
#include <WebServer.h>//Standard lib. (Used for client browser)
WebServer server(80);//Create object for the webserver on port 80.
//Webserver end.

#include "webSite.h"//Website with html, css and javascript.
#include "parsing_helpers.h"//Helper functions for parsing recieved data.

//WebSocket start.
#include <WebSocketsServer.h>//Version 2.4.0 (Is available in Manage Libraries) by Markus sattler.
WebSocketsServer webSocket = WebSocketsServer(81);//Create object for the WebSocketsServer on port 81.
//WebSocket end.

#include <DigiHz_EasyTransfer.h>//Include the library.

DigiHz_EasyTransfer myTransfer(false);//Create object with no debug and rx buffer default size 254 bytes.
//DigiHz_EasyTransfer myTransfer(true);//Create object with debug and rx buffer default size 254 bytes.

bool chunksRecieveStarted = false;//Init flag.
uint16_t currentChunkIdRecieved;//The current chunk id we recieve.
uint16_t previousChunkIdRecieved;//For catching duplicate reads from the rx buffer.
unsigned long firstChunkRecieved;//Will store when the first chunk was recieved.

//NOTE:The value off timeoutRecieveNextChunk you have to time for yourself in your own final code.
//NOTE:Generally you set the timeoutRecieveNextChunk value to ESP32-Cams txReties * txAckTimeout plus txChunkWaitTimeBetweenChunks.
const uint16_t timeoutRecieveNextChunk = 300;//How long time we wait maximum for next chunk to be recieved. (Value in milli seconds).
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

void freeRamHeapEsp32(){//Esp32 , Esp32-cam
  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}

void sendChunkToSite(uint16_t &numChunks, uint32_t &chunksTotalBytes, uint16_t &chunkId, uint8_t &chunkActualBytes, const uint8_t chunkDataArray[]){
  unsigned long timeStart = micros();

  //Serial.print(F("Sending chunk:"));Serial.println(chunkId);
  
  if (chunkId == 1){//If first chunk.
    webSocket.sendTXT(0, "numChunks=" + String(numChunks));//Send to first client.
    webSocket.sendTXT(0 ,"chunksTotalBytes=" + String(chunksTotalBytes));//Send to first client.
  }
  
  webSocket.sendTXT(0, "chunkId=" + String(chunkId));//Send to first client.
  webSocket.sendTXT(0, "chunkActualBytes=" + String(chunkActualBytes));//Send to first client.
  webSocket.sendBIN(0, chunkDataArray, chunkActualBytes);//Send to first client.
  
  /*
  Serial.print(F("Chunk data:"));
  for (uint8_t i = 0; i < chunkActualBytes; i++){
    if (chunkDataArray[i] < 16){
      Serial.print(F("0"));
    }
    Serial.print(chunkDataArray[i], HEX);
    if (i != chunkActualBytes - 1){//If not last byte.
      Serial.print(F(", "));
    }
  }
  Serial.println();
  */
  
  unsigned long timeEnd = micros();
  //Serial.print(F("Time to send chunk "));Serial.print(chunkId);Serial.print(F(" = "));Serial.print(timeEnd - timeStart);Serial.println(F(" micros seconds"));
}

//Webserver start.
void serveIndexFile(){//Serve the webSite.h file to the client.
  server.sendContent_P(indexHtmlEn);//Serve the webpage.
}
//Webserver end.

//WebSocket start.
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){

  String payloadString = (const char *)payload;
  //Serial.println("payload: '"+payloadString+"', client: "+(String)num);//Debug what we recieved from client.

  if (type == WStype_DISCONNECTED){//If a client is disconnected, then type == WStype_DISCONNECTED
    Serial.println("Client " + String(num) + " disconnected.");
  } else if (type == WStype_CONNECTED){//If a client is connected, then type == WStype_CONNECTED
    if (num != 0){//To many clients trying to connect. (We only allow 1 client to connect).
      webSocket.sendTXT(num, "toManyConnections=true");//Send a message to the client.
      Serial.print("Blocked client ");Serial.println(num);
      webSocket.disconnect(num);//Disconnect this client.
    } else {
      webSocket.sendTXT(num, "toManyConnections=false");//Send to first client.
      Serial.println("Client " + String(num) + " connected");
    }
  } else if (type == WStype_TEXT){//If a client has sent data, then type == WStype_TEXT

    //Split incoming string into key value pair start.
    byte separator = payloadString.indexOf('=');
    String var = payloadString.substring(0, separator);
    String val = payloadString.substring(separator + 1);
    //Split incoming string into key value pair end.

    //Process incoming data start.
    if (var == "requestImage"){
      //Send serial request to ESP32-Cam.
      uint8_t flashLedOnTime = atoi(val.c_str());//Convert string to uint8_t.
      if (myTransfer.sendData(details(flashLedOnTime), 11, true)){//Send var flashLedOnTime as identifier 11 and wait for ack.
        //Send websocket message back that we are preparing to get the image.
        webSocket.sendTXT(num, "acquiringImage=true");//Send to first client.
      } else {
        //Send websocket message back that we could not send the request.
        webSocket.sendTXT(num, "acquiringImage=false");//Send to first client.
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
    } else if (var == "rebootEsp32Cam"){
      //Send serial request to ESP32-Cam.
      bool rebootEsp32Cam = true;//Set flag.
      if (myTransfer.sendData(details(rebootEsp32Cam), 13, true)){//Send var rebootEsp32Cam as identifier 13 and wait for ack.
        //Send websocket message back that we are rebooting the ESP32-Cam.
        webSocket.sendTXT(num, "rebootingEsp32Cam=true");//Send to first client.
      } else {
        //Send websocket message back that we could not send the request.
        webSocket.sendTXT(num, "rebootingEsp32Cam=false");//Send to first client.
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
    }
    //Process incoming data end.
    
  } else if (type == WStype_ERROR){//Websocket connection error.
    Serial.println(F("Websocket ERROR!"));
  }
}
//WebSocket end.

void setup() {
  // put your setup code here, to run once:
  myTransferPort.begin(myTransferPortBaudRate);//Must be same baud rate on esp32cam_send_capture_with_ack_to_esp32.
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();

  //Wifi start.
  //NOTE:192.168.4.1 is default ip for AP.
  //Setting the ESP32 as an access point.
  Serial.print(F("Setting up AP ("));Serial.print(apSsid);Serial.println(F(")..."));
  #if useApPassword
    Serial.print(F("AP password:"));Serial.println(apPassword);
    WiFi.softAP(apSsid, apPassword);
  #else
    WiFi.softAP(apSsid);
  #endif
  IPAddress IP = WiFi.softAPIP();
  Serial.print(F("AP IP address:"));Serial.println(IP);
  //Wifi end.
  
  //Webserver start.
  server.begin();//Start the webserver.
  server.on("/", serveIndexFile);//Add route. (Serve the webSite.h file to the client).
  //Webserver end.

  //WebSocket start.
  webSocket.begin();//Start the websocket.
  webSocket.onEvent(webSocketEvent);//Define a callback function -> What does the ESP32 need to do when an event from the websocket is received? -> Run function "webSocketEvent()"
  //WebSocket end.

  Serial.println();
  freeRamHeapEsp32();
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
  //Webserver start.
  server.handleClient();//Monitor requests.
  //Webserver end.

  //WebSocket start.
  webSocket.loop();//Update function for the webSockets.
  //WebSocket end.
  
  
  if (myTransfer.receiveData()){//If we recieved anything valid.
    //Serial.println();
    //Serial.print(F("Recieved identifier "));Serial.println(myTransfer.rxIdentifier);
    //Serial.print(F("Recieved bytes "));Serial.println(myTransfer.rxBufferLength);
    
    /*
    //myTransfer.rxBuffer[0] to myTransfer.rxBuffer[myTransfer.rxBufferLength] is the data we recieved.
    Serial.println();Serial.println(F("rx buffer start."));
    for (uint8_t i = 0; i < myTransfer.rxBufferLength; i++){
      Serial.print(F("byte "));Serial.print(i + 1);Serial.print(F(" = "));Serial.println(myTransfer.rxBuffer[i]);
    }
    Serial.println(F("rx buffer end."));Serial.println();
    */

    if (myTransfer.rxIdentifier == 10){//txChunkData   
      uint16_t chunkDataRecievedNumChunks = processBytes2Uint16_t(myTransfer.rxBuffer, 0);//numChunks (2 bytes)//Byte 1 and 2.
      uint32_t chunkDataRecievedTotalBytes = processBytes2Uint32_t(myTransfer.rxBuffer, 2);//chunksTotalBytes (4 bytes) //Byte 3, 4 ,5  and 6.
      uint16_t chunkDataRecievedId = processBytes2Uint16_t(myTransfer.rxBuffer, 6);//chunkId (2 bytes)//Byte 7 and 8.
      uint8_t  chunkDataRecievedActualBytes = myTransfer.rxBuffer[8];//chunkActualBytes (1 byte) //Byte 9.
      
      uint8_t tempBuffer[chunkDataRecievedActualBytes];//Reserve a temp buffer for our chunk data to send to the client.
      //Serial.print(F("size of temp buffer:"));Serial.println(sizeof(tempBuffer));
      
      if (chunkDataRecievedId == 1){//If it is the first chunk.
        firstChunkRecieved = millis();//Store when the first chunk was recieved.
        previousChunkRecieved = millis();//Store when a chunk was recieved.
        if (!chunksRecieveStarted){
          chunksRecieveStarted = true;//Set flag.
          currentChunkIdRecieved = chunkDataRecievedId;
          previousChunkIdRecieved = 0;//Set value.
          //Serial.println(F("Struct parsed with parsing helpers."));
          //Serial.print(F("chunkId:"));Serial.println(chunkDataRecievedId);
          //Serial.println(F("Got the first chunk"));
          //Serial.print(F("chunk data bytes recieved:"));Serial.print(chunkDataRecievedActualBytes);Serial.println(F(" bytes."));
          //Serial.print(F("Chunk data recieved:"));
          for (uint8_t i = 9; i < chunkDataRecievedActualBytes + 9; i++){//chunkData  
            tempBuffer[i - 9] = myTransfer.rxBuffer[i];//Copy the actual chunk data to our temp buffer.
            //if (myTransfer.rxBuffer[i] < 16){
              //Serial.print(F("0"));
            //}
            //Serial.print(myTransfer.rxBuffer[i], HEX);
            //if (i - 9 != chunkDataRecievedActualBytes - 1){//If not last byte.
              //Serial.print(F(", "));
            //}
          }
          //Serial.println();  
          sendChunkToSite(chunkDataRecievedNumChunks, chunkDataRecievedTotalBytes, chunkDataRecievedId, chunkDataRecievedActualBytes, tempBuffer);
        } else {//Duplicate read of rx buffer.
          previousChunkRecieved = millis();//Update timeout time.
          //Serial.print(F("chunkId:"));Serial.println(chunkDataRecievedId);
          //Serial.println(F("Duplicate read of first chunk (ignoring it.)"));
        }
      } else {//Not first chunk.
        if (chunksRecieveStarted){//If we recieved the first chunk.
          previousChunkIdRecieved++;//Increase value by 1.
          currentChunkIdRecieved = chunkDataRecievedId;
          if (currentChunkIdRecieved - 1 == previousChunkIdRecieved){//If it is the next chunk we read.
            previousChunkRecieved = millis();//Store when a chunk was recieved.
            //Serial.print(F("chunkId:"));Serial.println(chunkDataRecievedId);
            //Serial.print(F("chunk data bytes recieved:"));Serial.print(chunkDataRecievedActualBytes);Serial.println(F(" bytes."));
            //Serial.print(F("Chunk data recieved:"));
            for (uint8_t i = 9; i < chunkDataRecievedActualBytes + 9; i++){//chunkData
              tempBuffer[i - 9] = myTransfer.rxBuffer[i];//Copy the actual chunk data to our temp buffer.
              //if (myTransfer.rxBuffer[i] < 16){
                //Serial.print(F("0"));
              //}
              //Serial.print(myTransfer.rxBuffer[i], HEX);
              //if (i - 9 != chunkDataRecievedActualBytes - 1){//If not last byte.
                //Serial.print(F(", "));
              //}
            }
            //Serial.println();
            sendChunkToSite(chunkDataRecievedNumChunks, chunkDataRecievedTotalBytes, chunkDataRecievedId, chunkDataRecievedActualBytes, tempBuffer);
            if (currentChunkIdRecieved == chunkDataRecievedNumChunks){//Check if we got the last chunk.  
              unsigned long lastChunkRecieved = millis();//Store when the last chunk was recieved.
              webSocket.sendTXT(0, "chunksRecieved=true");//Send to first client.
              Serial.println();
              Serial.println(F("********************************************************"));
              Serial.println(F("Recieved the last chunk. (All chunks recieved properly.)"));
              Serial.println(F("********************************************************"));
              Serial.print(F("Chunk data total bytes recieved:"));Serial.print(chunkDataRecievedTotalBytes);Serial.println(F(" bytes."));
              Serial.print(F("Time to recieve chunks:"));Serial.print(lastChunkRecieved - firstChunkRecieved);Serial.println(F(" milli seconds"));
              chunksRecieveStarted = false;//Reset flag.
              Serial.println();
              freeRamHeapEsp32();
              Serial.println();
            }
          } else if (currentChunkIdRecieved == previousChunkIdRecieved){//If it is a duplicate read of rx buffer.
            previousChunkRecieved = millis();//Update timeout time.
            //Serial.print(F("chunkId:"));Serial.println(chunkDataRecievedId);
            //Serial.println(F("Duplicate read of chunk (ignoring it.)"));
            //Roll back and just ignore this read and continue.
            previousChunkIdRecieved--;//Roll back.  
          }
        }
      }
    } else if (myTransfer.rxIdentifier == 11){//captureFrameAndCopyError
      Serial.println(F("Recieved captureFrameAndCopyError!"));
      webSocket.sendTXT(0, "captureFrameAndCopyError=true");//Send to first client.
      chunksRecieveStarted = false;//Reset flag.
    } else if (myTransfer.rxIdentifier == 12){//cameraInitializingError
      Serial.println(F("Recieved cameraInitializingError!"));
      webSocket.sendTXT(0, "cameraInitializingError=true");///Send to first client.
      chunksRecieveStarted = false;//Reset flag.
    } else {
      Serial.print(F("Recieved unknown identifier:"));Serial.println(myTransfer.rxIdentifier);
    }
    //Serial.println();
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
      webSocket.sendTXT(0, "chunkRecieveFailure=true");//Send to first client.
      chunksRecieveStarted = false;//Reset flag.
      Serial.println(F("ERROR:TIMEOUT RECIEVING CHUNK!"));
      Serial.println(F("ERROR:Did not catch the next chunk packet in time. (packet lost or skipped)"));
      Serial.print(F("Skipped after chunkId:"));Serial.println(previousChunkIdRecieved + 1);
      Serial.println(F("Maybe you should increase the timeoutRecieveNextChunk value?"));
      Serial.println();

      //Send serial request to ESP32-Cam.
      bool timeoutError = true;//Set flag.
      myTransfer.sendData(details(timeoutError), 14, true);//Send var timeoutError as identifier 14 and wait for ack.
      Serial.print(F("timeoutError "));Serial.println(F(" sent and we dont care if we recieve the ack or not."));
    }
  }
  
}
