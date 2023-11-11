//Version 1.0.7
#ifndef DigiHz_EasyTransfer_h
  #define DigiHz_EasyTransfer_h
  
  //Make it a little prettier on the front end. 
  #define details(name) (byte*)&name, sizeof(name)
  
  #if (ARDUINO >=100)
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
  
  #include "Stream.h"

  class DigiHz_EasyTransfer {
    public:
      const uint8_t rxBufferMinSize = 1;
      static const uint8_t rxBufferMaxSize = 254;
      const uint8_t txMaxSize = 254;

      //Constructor.
      DigiHz_EasyTransfer(bool debug = false,  uint32_t rxBufferSize = rxBufferMaxSize);
      /*
      In constructor, we use a bigger uint than really needed for rxBufferSize.
      Thats because we want to make it easier for the end user to not have to handle with overflow problems.
      It will use 3 bytes more memory, but i think it is better to do it this way.
      */

      //tx and rx start.
      bool begin(Stream *theStream, uint8_t flushRxPortBufferWaitTime = 0, Stream *debugPort = NULL);
      void setHeaders(byte header1 = 0x06, byte header2 = 0x85);
      //tx and rx end.
  
      //tx start.
      enum txStatusCode : uint8_t {
        TX_STATUS_OK          = 0,  //Success.
        TX_STATUS_DATA_TO_BIG = 1,  //Exceeded the max size of 254 bytes.
        TX_INVALID_IDENTIFIER = 2,  //Invalid id, 0 is reserved for ack.
        TX_STATUS_ACK_TIMEOUT = 3,  //Timeout in communication.
        TX_STATUS_UNKNOWN     = 255 //Unknown status.
      };
      void setTxRetries(uint8_t txRetries = 5);
      void setTxAckTimeout(uint16_t txAckTimeout = 50);
      bool sendData(uint8_t *, uint32_t, uint8_t identifier = 1, bool requestAck = false);
      /*
      In sendData method, we use a bigger uint than really needed for txLength.
      Thats because we want to make it easier for the end user to not have to handle with overflow problems.
      It will use 3 bytes more memory, but i think it is better to do it this way.
      */
      uint8_t getTxMaxSize();
      uint8_t getTxStatus();
      uint8_t getTxAttemptsDone();
      //tx end.
  
      //rx start.
      enum rxStatusCode : uint8_t {
        RX_STATUS_OK                    = 0,  //Success.
        RX_STATUS_BUFFER_TO_SMALL       = 1,  //Buffer size set to small.
        RX_STATUS_DATA_CHECKSUM_FAILED  = 2,  //Data checksum failed.
        RX_STATUS_ACK_CHECKSUM_FAILED   = 3,  //Ack checksum failed.
        RX_STATUS_ACK_RECIEVED          = 254,//Recieved ack ok.
        RX_STATUS_UNKNOWN               = 255 //Unknown status.
      };
      void clearRxBuffer();
      bool receiveData();
      uint8_t * rxBuffer; //Address for temporary storage and recieve buffer.
      uint8_t rxBufferLength;//Length of the buffer available to end user.
      uint8_t rxIdentifier;//Identifier of the buffer available to end user.
      uint8_t getRxBufferSize();
      uint8_t getRxStatus();
      void clearRxBufferOnRecieve(bool clearRxBufferOnRecieve = false);
      void flushRxPortBuffer(uint8_t = 0);
      void setFlushRxPortBufferWaitTime(uint8_t = 0);
      //rx end.
      
    private:
      //https://github.com/esp8266/Arduino/issues/7201
      //https://stackoverflow.com/questions/35413821/how-to-fix-this-array-used-as-initializer-error/35414564#35414564
      //const char _version[8] = {"1.0.7"};//Does not compile on 8266
      //const char _version[6] = {'1.0.7'};//This compiles on 8266, but only shows 7, not 1.0.7
      const char _version[6] = {'1','.','0','.','7','\0'};// ugly :( But... This compiles on 8266 correctly.
      
      //tx and rx start.
      bool _debug;
      Stream *_stream;
      Stream *_debugPort;
      byte _header1 = 0x06;
      byte _header2 = 0x85;
      unsigned long _txRxStartMeasure;
      unsigned long _txRxEndMeasure;
      unsigned long _txRxAckStartMeasure;
      unsigned long _txRxAckEndMeasure;
      //tx and rx end.
  
      //tx start.
      bool _txRequestAck;
      uint16_t _txAckTimeout = 50;
      uint8_t _txIdentifier;
      uint8_t _txRetries = 5;
      uint8_t * _txAddress;  //Address of struct or var or array.
      uint8_t _txSize;       //Size of struct or var or array.
      unsigned long _txWaitForAckStart;
      bool _txAckRecieved = false;
      uint8_t _txChecksum;
      uint8_t _txStatus = 0;//0 = success.
      bool _clearRxBufferOnRecieve = false;
      uint8_t _txAttemptsDone;
      //tx end.
  
      //rx start.
      uint32_t _rxBufferSize;
      /*
      We use a bigger uint than really needed for _rxBufferSize.
      Thats because we want to make it easier for the end user to not have to handle with overflow problems.
      It will use 3 bytes more memory, but i think it is better to do it this way.
      */
      uint8_t _rxLength;    //RX packet length according to the packet.
      uint8_t _rxIdentifier;
      bool _rxSendAck;
      uint8_t _rxArrayInx;  //Index for RX parsing buffer.
      uint8_t _rxCalculatedChecksum;     //Calculated Checksum.
      bool _rxSendTheAck(uint8_t *, uint8_t);
      uint8_t _rxStatus = 0;//0 = success.
      uint8_t _flushRxPortBufferWaitTime = 0;
      //rx end.
  };
#endif
