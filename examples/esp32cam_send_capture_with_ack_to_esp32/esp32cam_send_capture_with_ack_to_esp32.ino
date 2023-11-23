/*
https://github.com/DigiHzData/DigiHz_EasyTransfer
This example sends captured image chunk data with ack to the recieving unit (esp32_recieve_capture_with_ack_from_esp32cam).
We remarked a lot of serial prints in the sketch so the transfer will go in the supposed speed.
But we kept it there for you to debug easier if you want to.

Step1.
Upload this example to your mcu (ESP32-Cam).
Step2.
Connect myTransferPortSerialRxPin to the recievers myTransferPortSerialTxPin.
Connect myTransferPortSerialTxPin to the recievers myTransferPortSerialRxPin.
*/

#define myDebugPortBaudRate 115200//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//IMPORTANT:myTransferPortBaudRate must be same on esp32_recieve_capture_with_ack_from_esp32cam
//#define myTransferPortBaudRate 38400//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//#define myTransferPortBaudRate 115200//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
#define myTransferPortBaudRate 230400//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.
//#define myTransferPortBaudRate 460800//9600, 19200, 38400, 57600, 74880, 115200 //Try other baud rates that suits YOUR needs and see the diference in transfer speed / loop time.

//We use the second hardware port on ESP32-CAM, not software serial.
#include <HardwareSerial.h>
HardwareSerial myTransferPort(2);//UART2 Serial2 //Create hardware serial object.
#define myTransferPortSerialRxPin 15//GPIO15 UART 2 Recieve pin.
#define myTransferPortSerialTxPin 13//GPIO13 UART 2 Transmit pin.

#include <DigiHz_EasyTransfer.h>//Include the library.

DigiHz_EasyTransfer myTransfer(false, 1);//Create object with no debug and rx buffer size of min 1 byte.
//DigiHz_EasyTransfer myTransfer(true, 1);//Create object with debug and rx buffer size of min 1 byte.

#define CAMERA_MODEL_AI_THINKER // Has PSRAM (// Select camera model)

// Pin definitions for CAMERA_MODEL_AI_THINKER start.
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
// Pin definitions for CAMERA_MODEL_AI_THINKER end.

#include "esp_camera.h"//Standard lib.
#include "soc/soc.h"//Standard lib. brownout detector
#include "soc/rtc_cntl_reg.h"//Standard lib. brownout detector
#include "driver/rtc_io.h"//Standard lib.

//Flash led start.
#define flashLedPin 4//NOTE:Flash light is GPIO 4
uint8_t flashLedOnTime;//Will be set in loop. //For how long time we keep the flash led on when capturing an image.
//Flash led end.

//If copyFrameBufferWithMalloc is true then we make a copy of the frame buffer dynamicly with malloc.
//If copyFrameBufferWithMalloc is false then we make a copy of the frame buffer to a global uint8_t array.
#define copyFrameBufferWithMalloc true

//Global vars we can use after we have captured and copied the image from the frame buffer.
size_t fbCopiedBufferLength;//Will have the length of the copied frame buffer.
#if copyFrameBufferWithMalloc
  //NOTE:Will use PSRAM for the copied frame.
  uint8_t *fbCopiedBuffer = NULL;//Pointer to copied frame buffer pixels.
#else
  //NOTE:Will use DRAM  for the copied frame.
  //IMPORTANT:Make sure you allocate enough bytes to hold the captured image.
  //NOTE:The bigger the sketch is, the less you can allocate.
  uint8_t fbCopiedBuffer[50000];//Reserve 50Kb for our copied frame buffer.
#endif

//For measure the capture start.
unsigned long captureStart;
unsigned long captureEnd;
unsigned long memcpyStart;
unsigned long memcpyEnd;
//For measure the capture end.

bool cameraInitializedOk = false;//Init flag.
bool initCapture = false;//Init flag.

bool chunksSendingStarted = false;//Init flag.
uint16_t currentChunkId = 0;//Init counter.

unsigned long txChunksStartMeasure;//For measure time it takes to send all chunks.
unsigned long txChunksEndMeasure;//For measure time it takes to send all chunks.

//NOTE:The value off txChunkWaitTimeBetweenChunks you have to time for yourself in your own final code.
const uint16_t txChunkWaitTimeBetweenChunks = 0;//Time we wait between each chunk we send. (Value in milli seconds).
unsigned long previousMillisSentChunkData = 0;//Init //Store when we last sent our chunk data packet.

//NOTE:The higher value chunksMaxBytes have, the less latency time you will have.
//const uint8_t chunksMaxBytes = 64;//Max number of bytes we send at a time. (64 + 9 = 73 bytes)
//const uint8_t chunksMaxBytes = 128;//Max number of bytes we send at a time. (128 + 9 = 137 bytes)
const uint8_t chunksMaxBytes = 245;//Max number of bytes we send at a time. (245 + 9 = 254 bytes)
//INFO:Max image we can send in size is numChunks(65535) * chunkData[chunksMaxBytes];
//INFO:If chunksMaxBytes is set to 64 then we get a max size of 65535 * 64 = 4 194 240 bytes
//INFO:If chunksMaxBytes is set to 128 then we get a max size of 65535 * 128 = 8 388 480 bytes
//INFO:If chunksMaxBytes is set to 245 then we get a max size of 65535 * 245 = 16 056 075 bytes

uint32_t chunksTotalBytes;//Will be set in loop. //The total number of bytes we are going to send.
uint16_t numChunks;//Will be calculated in loop.
uint8_t chunksRemainerBytes;//Will be calculated in loop.

//Data packets we send start.
struct SEND_CHUNKS_DATA_STRUCTURE {//Data we send. 9 bytes chunk meta data + chunksMaxBytes bytes chunk data.
  uint16_t numChunks;//(2 bytes) //Will have the total number of chunks in it.
  uint32_t chunksTotalBytes;//(4 bytes) //Will have the total number of bytes for all the chunks in it.
  uint16_t chunkId;//(2 bytes) //Will have the current chunk id in it.
  uint8_t chunkActualBytes;//(1 byte) //Will have the size in bytes for the current chunk in it.
  uint8_t chunkData[chunksMaxBytes];//Will have the chunk data in it.
//};//Only using this may not make it work with mixed boards.
} __attribute((__packed__)) packet_chunks_send;//This fixes problem with struct size difference between mixed boards. https://github.com/esp8266/Arduino/issues/1825
SEND_CHUNKS_DATA_STRUCTURE txChunkData;//No specific default values for the struct. //Use this if you don't care about the initial values in the struct.
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

bool initCameraFrameBuffer(){
  camera_fb_t * fb = esp_camera_fb_get();//Setup frame buffer (Capture a dummy image).
  if (!fb) {
    esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
    return false;
  } else {
    Serial.print(F("Image width:"));Serial.print(fb->width);Serial.println(F(" pixels."));
    Serial.print(F("Image height:"));Serial.print(fb->height);Serial.println(F(" pixels."));
    esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
    return true;
  }
}

void resetStuff(){
  currentChunkId = 0;//Reset counter.
  chunksSendingStarted = false;//Reset flag.
  #if copyFrameBufferWithMalloc
    free(fbCopiedBuffer);//SUPER IMPORTANT! Free what we allocated with malloc after we done with it before we capture the next image.
    fbCopiedBuffer = NULL;//Set pointer to NULL.
  #else
    memset(fbCopiedBuffer, 0, sizeof(fbCopiedBuffer));//Clear the array if you want to.
  #endif
}

bool captureFrameAndCopy(){
  if (flashLedOnTime != 0){
    digitalWrite(flashLedPin, HIGH);//Turn the flash led on.
    delay(flashLedOnTime);//Give some time to light things up.
  }
  captureStart = micros();
  camera_fb_t * fb = esp_camera_fb_get();//Setup frame buffer (Capture an image).
  if (flashLedOnTime != 0){
    digitalWrite(flashLedPin, LOW);//Turn the flash led off.
  }
  if (!fb) {
    Serial.print(F("ERROR:Camera frameBufferError"));Serial.println();
    esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
    return false;
  } else {
    fbCopiedBufferLength = fb->len;//Store the frame buffer length for use later.
    #if copyFrameBufferWithMalloc
      fbCopiedBuffer = (uint8_t*)malloc(fbCopiedBufferLength);//Reserve memory dynamicly for our copied image.
      if (fbCopiedBuffer == NULL){//Check if malloc reserved the bytes or not.
        Serial.println(F("ERROR:malloc failed! Could not allocate enough memory! Buffer not copied!"));
        Serial.print(F("Captured image size:"));Serial.print(fbCopiedBufferLength);Serial.println(F(" bytes."));
        esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
        fb = NULL;
        return false;
      }
      memcpyStart = micros();
      memcpy(fbCopiedBuffer, fb->buf, fbCopiedBufferLength);//Copy frame.
    #else
      if (fbCopiedBufferLength > sizeof(fbCopiedBuffer)){//Check if our reserved buffer is big enough.
        Serial.println(F("ERROR:Copy buffer failed! Captured image to big for copy buffer! Buffer not copied!"));
        Serial.print(F("fbCopiedBuffer is only "));Serial.print(sizeof(fbCopiedBuffer));Serial.println(F(" bytes."));
        Serial.print(F("Captured image size:"));Serial.print(fbCopiedBufferLength);Serial.println(F(" bytes."));
        esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
        fb = NULL;
        return false;
      }
      memcpyStart = micros();
      memcpy(fbCopiedBuffer, fb->buf, fbCopiedBufferLength);//Copy frame.
    #endif
    memcpyEnd = micros();
    esp_camera_fb_return(fb);//Return the frame buffer back to the driver for reuse.
    fb = NULL;
    captureEnd = micros();
    return true;
  } 
}

void freeRamHeapEsp32(){//Esp32 , Esp32-cam
  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}

//NOTE:An captured jpeg image starts with //FF D8 and ends with FF D9
void printArrayHEX(uint8_t *buff, uint32_t len){
  //https://forum.arduino.cc/t/the-jpeg-format-of-esp32-camera/915461/18
  uint8_t columnCounter = 0;//Init counter.
  uint32_t index;
  uint8_t buffdata;
  for (index = 0; index < len; index++){
    columnCounter++;//Increase counter.
    buffdata = buff[index];
    if (buffdata < 16){
      Serial.print(F("0"));
    }
    Serial.print(buffdata, HEX);
    Serial.print(F(" "));
    if (columnCounter == 16){//If we printed 16 columns.
      Serial.println();
      columnCounter = 0;//Reset.
    }
  }
}

bool sendChunk(const uint16_t chunkId){//Function for sending chunks one by one from the frame buffer.

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

  memcpy(&txChunkData.chunkData, &fbCopiedBuffer[chunksMaxBytes * chunkId], txChunkData.chunkActualBytes);//Copy txChunkData.chunkActualBytes bytes to txChunkData.chunkData from copied frame buffer.
  //Serial.print(F("Sending chunk "));Serial.print(chunkId + 1);Serial.print(F(" of "));Serial.println(numChunks);
  
  /*
  Serial.print(F("Chunk data:"));
  for (uint8_t i = 0; i < txChunkData.chunkActualBytes; i++){//chunkData
    if (txChunkData.chunkData[i] < 16){
      Serial.print(F("0"));
    }
    Serial.print(txChunkData.chunkData[i], HEX);
    if (i != txChunkData.chunkActualBytes - 1){//If not last byte.
      Serial.print(F(", "));
    }
  }
  Serial.println();
  */
  
  if (myTransfer.sendData(details(txChunkData), 10, true)){//Send struct txChunkData as identifier 10 and wait for ack.
    //Serial.print(F("Chunk packet "));Serial.print(chunkId + 1);Serial.println(F(" sent and ack recieved."));
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
      //printArrayHEX(fbCopiedBuffer, fbCopiedBufferLength);//Debug copied frame buffer.
      resetStuff();
      Serial.println();
      freeRamHeapEsp32();
      Serial.println();
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

void setup() {
  // put your setup code here, to run once:
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//Disable brownout detector.
  
  myTransferPort.begin(myTransferPortBaudRate, SERIAL_8N1, myTransferPortSerialRxPin, myTransferPortSerialTxPin);//Must be same baud rate on esp32_recieve_capture_with_ack_from_esp32cam.
  Serial.begin(myDebugPortBaudRate);
  myTransfer.begin(&myTransferPort, 10, &Serial);//Transfer port, 10 mS rx port buffer flush, with debug port.
  configureMyTransfer();//Configure if you need to tweak things. //Must be called after myTransfer.begin.
  Serial.println();
  Serial.print(F("Struct txChunkData occupies "));Serial.print(sizeof(txChunkData));Serial.println(F(" bytes."));
  Serial.print(F("chunksMaxBytes:"));Serial.println(chunksMaxBytes);
  Serial.println();

  //Flash led start.
  pinMode(flashLedPin, OUTPUT);//IMPORTANT!
  //Flash led end.
  
  //Camera start.
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  //config.frame_size = FRAMESIZE_96X96;//96*96 pixels.
  //config.frame_size = FRAMESIZE_QVGA;//320*240 pixels.
  //config.frame_size = FRAMESIZE_CIF;//400*296 pixels.
  //config.frame_size = FRAMESIZE_VGA;//640*480 pixels.
  //config.frame_size = FRAMESIZE_SVGA;//800*600 pixels.
  //config.frame_size = FRAMESIZE_XGA;//1024*768 pixels.
  //config.frame_size = FRAMESIZE_SXGA;//1280 x 1024 pixels.
  config.frame_size = FRAMESIZE_UXGA;//1600 x 1200 pixels.
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    //config.frame_size = FRAMESIZE_96X96;//96*96 pixels.
    //config.frame_size = FRAMESIZE_QVGA;//320*240 pixels.
    //config.frame_size = FRAMESIZE_CIF;//400*296 pixels.
    //config.frame_size = FRAMESIZE_VGA;//640*480 pixels.
    config.frame_size = FRAMESIZE_SVGA;//800*600 pixels.
    //config.frame_size = FRAMESIZE_XGA;//1024*768 pixels.
    //config.frame_size = FRAMESIZE_SXGA;//1280 x 1024 pixels.
    //config.frame_size = FRAMESIZE_UXGA;//1600 x 1200 pixels.
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  delay(250);//Short pause helps to ensure the I2C interface has initialised properly before attempting to detect the camera.
  
  //Camera init start.
  esp_err_t err = esp_camera_init(&config);//Initialize the camera.
  if (err != ESP_OK) {
    Serial.printf("ERROR:Camera init failed with error 0x%x", err);Serial.println();
  } else {
    Serial.print(F("Camera initialized ok"));Serial.println();
    cameraInitializedOk = true;//Set flag.
  }
  //Camera init end.
  
  if (cameraInitializedOk){
    sensor_t * s = esp_camera_sensor_get();
    //s->set_framesize(s, FRAMESIZE_96X96);//96*96 pixels.
    //s->set_framesize(s, FRAMESIZE_QVGA);//320*240 pixels.
    //s->set_framesize(s, FRAMESIZE_CIF);//400*296 pixels.
    s->set_framesize(s, FRAMESIZE_VGA);//640*480 pixels.
    //s->set_framesize(s, FRAMESIZE_SVGA);//800*600 pixels.
    //s->set_framesize(s, FRAMESIZE_XGA);//1024*768 pixels.
    //s->set_framesize(s, FRAMESIZE_SXGA);//1280 x 1024 pixels.
    //s->set_framesize(s, FRAMESIZE_UXGA);//1600 x 1200 pixels.
    
    s->set_hmirror(s, 1);// 0 = disable , 1 = enable
    s->set_vflip(s, 1);// 0 = disable , 1 = enable
    
    initCapture = initCameraFrameBuffer();//NOTE:The first capture takes longer time so we take a fake capture first.
    if (initCapture){
      Serial.print(F("Camera sensor ok"));Serial.println();
    } else {
      Serial.print(F("Camera sensor failed"));Serial.println();
    }
  } else {
    Serial.print(F("Camera sensor failed!"));Serial.println();
  }
  //Camera end.
  
  #if copyFrameBufferWithMalloc
    Serial.println(F("Frame buffer will be copied using malloc."));
  #else
    Serial.println(F("Frame buffer will be copied using global uint8_t array."));
  #endif
  Serial.println();

  freeRamHeapEsp32();
  Serial.println();
}

void loop() {
  // put your main code here, to run repeatedly:
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

    if (myTransfer.rxIdentifier == 11){//Request to capture an image.
      
      flashLedOnTime = myTransfer.rxBuffer[0];//flashLedOnTime (1 byte) //Byte 1.
        
      if (cameraInitializedOk){
        txChunksStartMeasure = millis();
        //Serial.println();
        //Serial.println(F("Capturing image..."));
        if (captureFrameAndCopy()){//If success.

          chunksTotalBytes = fbCopiedBufferLength;
          
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
          
          //Serial.print(F("Captured image size:"));Serial.print(fbCopiedBufferLength);Serial.println(F(" bytes."));
          chunksSendingStarted = true;//Set flag.

          //Serial.print(F("chunksMaxBytes:"));Serial.println(chunksMaxBytes);
          //Serial.print(F("chunksTotalBytes:"));Serial.print(chunksTotalBytes);Serial.println(F(" bytes."));
          //Serial.print(F("numChunks:"));Serial.println(numChunks);
          //Serial.print(F("chunksRemainerBytes:"));Serial.print(chunksRemainerBytes);Serial.println(F(" bytes."));
          
          //Serial.print(F("The memcpy took: "));Serial.print(memcpyEnd - memcpyStart);Serial.println(F(" us"));
          //Serial.print(F("The capture took: "));Serial.print(captureEnd - captureStart);Serial.println(F(" us"));
        } else {//captureFrameAndCopy Error.
          //Send a message back to the ESP32.
          bool captureFrameAndCopyError = true;//Set flag.
          if (myTransfer.sendData(details(captureFrameAndCopyError), 11, true)){//Send var captureFrameAndCopyError as identifier 11 and wait for ack.
            Serial.print(F("Error captureFrameAndCopyError "));Serial.println(F(" sent and ack recieved."));
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
        }
      } else {//Camera initializing error.
        //Send a message back to the ESP32.
        bool cameraInitializingError = true;//Set flag.
        if (myTransfer.sendData(details(cameraInitializingError), 12, true)){//Send var cameraInitializingError as identifier 12 and wait for ack.
          Serial.print(F("Error cameraInitializingError "));Serial.println(F(" sent and ack recieved."));
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
      } 
    } else if (myTransfer.rxIdentifier == 13){//Request to reboot this ESP32-Cam.
      if (myTransfer.rxBuffer[0] == true){//rebootEsp32Cam (1 byte) //Byte 1.
        Serial.println(F("Rebooting this ESP32-Cam..."));
        ESP.restart();//Hard reset the ESP32-Cam.
      }
    } else if (myTransfer.rxIdentifier == 14){//Timeout detected on esp32. 
      //NOTE:We might actually not get this identifier 14 from the ESP32 in this example, but an ERROR:Ack checksum failed! instead.
      Serial.println(F("timeoutError recieved."));
      resetStuff();
      Serial.println();
      freeRamHeapEsp32();
      Serial.println();
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
      //We stop the ongoing transfer if this error happens.
      resetStuff();
      Serial.println();
      freeRamHeapEsp32();
      Serial.println();
    }
  }

  if (cameraInitializedOk){
    if (chunksSendingStarted){//If we started the sending of the chunks.
      if (millis() - previousMillisSentChunkData >= txChunkWaitTimeBetweenChunks) {//Check if it is time to send chunk data.
        if (!sendChunk(currentChunkId)){//Failure sending chunk.
          resetStuff();
          Serial.println();
          freeRamHeapEsp32();
          Serial.println();
          Serial.println(F("Stopped sending chunks because of transmittion failure!"));
          Serial.println(F("Maybe you should increase the txChunkWaitTimeBetweenChunks value?"));
          Serial.println(F("Or decrease the value of chunksMaxBytes?"));
          Serial.println();
        }
        previousMillisSentChunkData = millis();
      }
    }
  }

}
