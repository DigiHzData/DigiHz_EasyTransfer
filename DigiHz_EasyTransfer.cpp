//Version 1.0.7
#include "DigiHz_EasyTransfer.h"

DigiHz_EasyTransfer::DigiHz_EasyTransfer(bool debug, uint32_t rxBufferSize) {
  _debug = debug;
  _rxBufferSize = rxBufferSize;
}

uint8_t DigiHz_EasyTransfer::getTxMaxSize(){
  return txMaxSize;
}

uint8_t DigiHz_EasyTransfer::getRxBufferSize(){
  return _rxBufferSize;
}

uint8_t DigiHz_EasyTransfer::getTxStatus(){
  if (_txStatus < 255){//Prevent end user from using TX_STATUS_UNKNOWN
    return _txStatus;
  } else {
    return NULL;
  }
}

uint8_t DigiHz_EasyTransfer::getRxStatus(){
  if (_rxStatus < 254){//Prevent end user from using RX_STATUS_ACK_RECIEVED and RX_STATUS_UNKNOWN
    return _rxStatus;
  } else {
    return NULL;
  }
}

uint8_t DigiHz_EasyTransfer::getTxAttemptsDone(){
  return _txAttemptsDone;
}

bool DigiHz_EasyTransfer::begin(Stream *theStream, uint8_t flushRxPortBufferWaitTime, Stream *debugPort) {
  _stream = theStream;
  _debugPort = debugPort;
  if (_debugPort == NULL){//No debug port specified.
    _debug = false;
  }
  if (_debug) {
    _debugPort->println(F("DBG:DigiHz_EasyTransfer begin method instantiated (created) successfully."));
    _debugPort->print(F("DBG:Version "));_debugPort->println(_version);
    _debugPort->print(F("DBG:tx max size is:"));_debugPort->print(txMaxSize);_debugPort->println(F(" bytes."));
  }
  if (_rxBufferSize < rxBufferMinSize){
    if (_debug) {
      _debugPort->print(F("DBG:ERROR! rx buffer size was set to:"));_debugPort->print(_rxBufferSize);_debugPort->print(F(" bytes. (MINIMUM is "));_debugPort->print(rxBufferMinSize);_debugPort->println(F(" bytes)."));
      _debugPort->print(F("DBG:OVERIDE! rx buffer size is set to:"));_debugPort->print(rxBufferMinSize);_debugPort->println(F(" byte."));
    }
    _rxBufferSize = rxBufferMinSize;
  } else if (_rxBufferSize > rxBufferMaxSize){
    if (_debug) {
      _debugPort->print(F("DBG:ERROR! rx buffer size was set to:"));_debugPort->print(_rxBufferSize);_debugPort->print(F(" bytes. (MAXIMUM is "));_debugPort->print(rxBufferMaxSize);_debugPort->println(F(" bytes)."));
      _debugPort->print(F("DBG:OVERIDE! rx buffer size is set to:"));_debugPort->print(rxBufferMaxSize);_debugPort->println(F(" bytes."));
    }
    _rxBufferSize = rxBufferMaxSize;//Overide user's setting of rx buffer size.
  } else {
    if (_debug) {
      _debugPort->print(F("DBG:rx buffer size is set to:"));_debugPort->print(_rxBufferSize);_debugPort->println(F(" bytes."));
    }
  }
  rxBuffer = (uint8_t*) malloc(_rxBufferSize + 1);//Dynamic creation of rx buffer in RAM.

  _flushRxPortBufferWaitTime = flushRxPortBufferWaitTime;
  if (_debug) {
    _debugPort->print(F("DBG:Flush rx port buffer wait time set to:"));_debugPort->print(_flushRxPortBufferWaitTime);_debugPort->println(F(" milli seconds."));
  }

  if (_flushRxPortBufferWaitTime > 0){
    flushRxPortBuffer(flushRxPortBufferWaitTime);//Flush the rx port buffer.
  }
  return true;
}

void DigiHz_EasyTransfer::setHeaders(byte header1, byte header2){
  _header1 = header1;
  _header2 = header2;
  if (_debug) {
    _debugPort->print(F("DBG:header 1 set to:0x"));_debugPort->println(_header1, HEX);
    _debugPort->print(F("DBG:header 2 set to:0x"));_debugPort->println(_header2, HEX);
  }
}

void DigiHz_EasyTransfer::clearRxBufferOnRecieve(bool clearRxBufferOnRecieve){
  _clearRxBufferOnRecieve = clearRxBufferOnRecieve;
  if (_debug) {
    _debugPort->print(F("DBG:clearRxBufferOnRecieve set to:"));
    if (_clearRxBufferOnRecieve){
      _debugPort->println(F("true"));
    } else {
      _debugPort->println(F("false"));
    }
  }
}

void DigiHz_EasyTransfer::setTxRetries(uint8_t txRetries){
  if (txRetries == 0){
    txRetries = 1;//Overide user's setting of tx retries.
  }
  _txRetries = txRetries;
  if (_debug) {
     _debugPort->print(F("DBG:tx retries set to:"));_debugPort->println(_txRetries);
  }
}

void DigiHz_EasyTransfer::setTxAckTimeout(uint16_t txAckTimeout){
  _txAckTimeout = txAckTimeout;
  if (_debug) {
    _debugPort->print(F("DBG:tx ack timeout set to:"));_debugPort->print(_txAckTimeout);_debugPort->println(F(" milli seconds."));
  }
}

void DigiHz_EasyTransfer::setFlushRxPortBufferWaitTime(uint8_t flushRxPortBufferWaitTime){
  _flushRxPortBufferWaitTime = flushRxPortBufferWaitTime;
  if (_debug) {
    _debugPort->print(F("DBG:Flush rx port buffer wait time set to:"));_debugPort->print(_flushRxPortBufferWaitTime);_debugPort->println(F(" milli seconds."));
  }
}

void DigiHz_EasyTransfer::clearRxBuffer(){
  memset(rxBuffer, 0, _rxBufferSize);//Clear the buffer.
}

void DigiHz_EasyTransfer::flushRxPortBuffer(uint8_t delayTime){
  if (_debug) {
     _debugPort->println(F("DBG:Flushing rx port buffer..."));
  }
  while (true){
    delay (delayTime);//Give data a chance to arrive.
    if (_stream->available()){//We received something, get all of it and discard it from the buffer.
      while (_stream->available()){
        _stream->read();
      }
      continue;//Stay in the main while loop.
    } else {
      break;//Nothing arrived for delayTime ms
    }
  }
  if (_debug) {
    _debugPort->println(F("DBG:Flushing of rx port buffer done."));
  }
}

bool DigiHz_EasyTransfer::sendData(uint8_t * ptr, uint32_t txLength, uint8_t txIdentifier, bool txRequestAck){
  if (txLength > txMaxSize){
    _txStatus = TX_STATUS_DATA_TO_BIG;
    return false;
  }
  if (txIdentifier == 0){
    _txIdentifier = 0;//Set identifier to 0.
    _txRequestAck = false;//Reset flag.
    _txStatus = TX_INVALID_IDENTIFIER;
    return false;
  } else {
    _txIdentifier = txIdentifier;
    _txRequestAck = txRequestAck;
  }
  uint8_t _numTxRetries;
  if (txRequestAck){//If we request ack, then we try this many times to send.
    _numTxRetries = _txRetries;
  } else {//We only try to send once.
    _numTxRetries = 1;
  }
  _txAttemptsDone = 0;//Reset counter.
  for (uint8_t i = 0; i < _numTxRetries; i++) {
    if (_debug) {
      _debugPort->println();
    }
    _txAttemptsDone++;//Increase counter.
    if (_numTxRetries > 1){
      if (_debug) {
        _debugPort->print(F("DBG:Send atttempt "));_debugPort->print(i + 1);_debugPort->print(F("/"));_debugPort->println(_numTxRetries);
      }
    }
    //Captures address and size of struct or var or array start.
    _txAddress = ptr;
    _txSize = txLength;//sizeOfPayload
    //Captures address and size of struct or var or array end.
    //Sends out struct or var or array in binary, with header(s), identifier, requestAck, sizeOfPayload, payload and checksum start.
    uint8_t txChecksum = _txSize;
    if (_debug) {
      _txRxStartMeasure = micros();
    }
    _stream->write(_header1);
    _stream->write(_header2);
    _stream->write(_txIdentifier);
    _stream->write(_txRequestAck);
    _stream->write(_txSize);
    for (int i = 0; i < _txSize; i++){
      txChecksum^=*(_txAddress + i);
      _stream->write(*(_txAddress + i));
    }
    _stream->write(txChecksum);
    //Sends out struct or var or array in binary, with header(s), identifier, requestAck, sizeOfPayload, payload and checksum end.
    _txChecksum = txChecksum;
    if (_debug) {
      _txRxEndMeasure = micros();
      _debugPort->print(F("DBG:Time to send data packet:"));_debugPort->print(_txRxEndMeasure - _txRxStartMeasure);_debugPort->println(F(" micro seconds."));
      _debugPort->print(F("DBG:tx data packet identifier:"));_debugPort->println(_txIdentifier);
      _debugPort->print(F("DBG:tx data packet length:"));_debugPort->print(_txSize);_debugPort->println(F(" bytes."));
      _debugPort->print(F("DBG:tx data packet checksum:"));_debugPort->println(_txChecksum);
    }
    if (_txRequestAck){//If we request ack.
      if (_debug) {
        _txRxAckStartMeasure = micros();
        _debugPort->println(F("DBG:Waiting for ack..."));
      }
      _txWaitForAckStart = millis();
      _txAckRecieved = false;//Set flag.
      while (_txWaitForAckStart + _txAckTimeout > millis()){
        delay(1);//Slow things down a bit, we dont need to check continiously. //NOTE:Not sure we are going to use this delay here.
        receiveData();
        if (_txAckRecieved){//Break out of while loop if we recieved the ack.
          break;
        }
        yield();
      }
      if (_txAckRecieved){//We got the ack.
        if (_debug) {
          _txRxAckEndMeasure = micros();
          _debugPort->print(F("DBG:Got the ack in:"));_debugPort->print(_txRxAckEndMeasure - _txRxAckStartMeasure);_debugPort->println(F(" micro seconds."));
          _debugPort->println();
        }
        _txAckRecieved = false;//Reset flag.
        _txStatus = TX_STATUS_OK;
        return true;
      } else {//We did not get the ack back within the timeout.
        if (_debug) {
          _txRxAckEndMeasure = micros();
          _debugPort->print(F("DBG:Gave up waiting for ack after:"));_debugPort->print(_txRxAckEndMeasure - _txRxAckStartMeasure);_debugPort->println(F(" micro seconds."));
          _debugPort->println();
        }
        if (_numTxRetries == i + 1){
          _txStatus = TX_STATUS_ACK_TIMEOUT;
          return false;
        }
      }
    } else {//Not requesting ack.
      if (_debug) {
        _debugPort->println();
      }
      if (_numTxRetries == 1){
        _txStatus = TX_STATUS_OK;
        return true;
      }
    }
  }
  _txStatus = TX_STATUS_UNKNOWN;
  return false;
}

// Private method for this class
bool DigiHz_EasyTransfer::_rxSendTheAck(uint8_t * ptr, uint8_t txLength){
  _txIdentifier = 0;//Set identifier to 0.
  _txRequestAck = false;//Reset flag.
  //Captures address and size of var start.
  _txAddress = ptr;
  _txSize = txLength;//sizeOfPayload
  //Captures address and size of var end.
  //Sends out the var in binary, with header(s), identifier, requestAck, sizeOfPayload, payload and checksum start.
  uint8_t txChecksum = _txSize;
  if (_debug) {
    _txRxStartMeasure = micros();
  }
  _stream->write(_header1);
  _stream->write(_header2);
  _stream->write(_txIdentifier);
  _stream->write(_txRequestAck);
  _stream->write(_txSize);
  txChecksum^=*(_txAddress);
  _stream->write(*(_txAddress));
  _stream->write(txChecksum);
  //Sends out the var in binary, with header(s), identifier, requestAck, sizeOfPayload, payload and checksum end.
  if (_debug) {
    _txRxEndMeasure = micros();
    _debugPort->print(F("DBG:Time to send ack packet:"));_debugPort->print(_txRxEndMeasure - _txRxStartMeasure);_debugPort->println(F(" micro seconds."));
  }
  return true;
}

bool DigiHz_EasyTransfer::receiveData(){
  //Start off by looking for the header bytes. If they were already found in a previous call, skip it.
  if (_rxLength == 0){
    //This size check may be redundant due to the size check below, but for now I'll leave it the way it is.
    if (_stream->available() >= 5){//header1, header2, identifier, requestAck, size 
      //This will block until a header1 is found or buffer size becomes less then 5.
      while (_stream->read() != _header1) {
        //This will trash any preamble junk in the serial buffer.
        //But we need to make sure there is enough in the buffer to process while we trash the rest.
        //If the buffer becomes too empty, we will escape and try again on the next call.
        if (_stream->available() < 5) {
          _rxStatus = RX_STATUS_UNKNOWN;
          return false;
        }
      }
      if (_stream->read() == _header2){
        _rxIdentifier = _stream->read();
        _rxSendAck = _stream->read();
        _rxLength = _stream->read();
      }
    }
  }
  //We get here if we already found the header bytes, the identifier, the requestAck and the size matched what we know, and now we are byte aligned.
  if (_rxLength != 0){  
    if (_clearRxBufferOnRecieve){
      clearRxBuffer();//Ensure that the rx buffer is empty! (Optional)
    }
    if (_rxLength > _rxBufferSize){//Catch buffer size under dimensioned!
      if (_debug) {
        _debugPort->println(F("DBG:ERROR, rx buffer size is set to small! Increase the rx buffer size."));
      }
      _rxLength = 0;//Reset so we can recieve next packet.
      _rxArrayInx = 0;
      _rxStatus = RX_STATUS_BUFFER_TO_SMALL;
      return false;
    }
    rxBufferLength = _rxLength;
    rxIdentifier = _rxIdentifier;
    while(_stream->available() && _rxArrayInx <= _rxLength){
      rxBuffer[_rxArrayInx++] = _stream->read();
    }
    if (_rxLength == (_rxArrayInx - 1)){//Got the whole packet.
      if (_debug) {
        _debugPort->println(F("DBG:Seem to have gotten the whole packet."));
      }
      //Last uint8_t is the checksum.
      _rxCalculatedChecksum = _rxLength;
      for (int i = 0; i < _rxLength; i++){
        _rxCalculatedChecksum^=rxBuffer[i];
      }
      if (_rxCalculatedChecksum == rxBuffer[_rxArrayInx - 1]){//Checksum is good.
        if (_rxIdentifier != 0){//If we recieved a data packet.
          if (_debug) {
            _debugPort->println(F("DBG:Recieved data packet."));
            _debugPort->print(F("DBG:rx identifier:"));_debugPort->println(_rxIdentifier);
            _debugPort->print(F("DBG:rx data packet checksum:"));_debugPort->println(rxBuffer[_rxArrayInx - 1]);
            _debugPort->println(F("DBG:rx data packet checksum OK."));
          }
          if (_rxSendAck){//If sender requested an ack.
            if (_debug) {
              _debugPort->println(F("DBG:Sending ack back."));
            }
	    if (_flushRxPortBufferWaitTime > 0){
              flushRxPortBuffer(_flushRxPortBufferWaitTime);//Flush the rx port buffer so we not get garbage.
            }
	    _rxSendTheAck(details(rxBuffer[_rxArrayInx - 1]));//Send the ack back.
            _rxSendAck = false;
          }
          _rxLength = 0;//Reset so we can recieve next packet.
          _rxArrayInx = 0;
          _rxStatus = RX_STATUS_OK;
          return true;
        } else {//We recieved an ack packet.
          if (_debug) {
            _debugPort->println(F("DBG:Recieved ack packet."));
            _debugPort->print(F("DBG:rx ack packet checksum:"));_debugPort->println(rxBuffer[0]);
            _debugPort->println(F("DBG:rx ack packet checksum OK."));
          }
          if (rxBuffer[0] == _txChecksum){//If tx data packet checksum and rx ack packet checksum matches.
            if (_debug) {
              _debugPort->println(F("DBG:tx data packet checksum and rx ack packet checksum matches."));
            }
            _txAckRecieved = true;
            _rxLength = 0;//Reset so we can recieve next packet.
            _rxArrayInx = 0;
            _rxStatus = RX_STATUS_ACK_RECIEVED;//NOTE:We will not get this back, but we set a status anyway.
            return true;
          } else {//rx ack packet checksum did not match tx data packet checksum.
            if (_debug) {
              _debugPort->println(F("DBG:ERROR, rx ack packet checksum did not match tx data packet checksum!"));
            } 
            _rxLength = 0;//Reset so we can recieve next packet.
            _rxArrayInx = 0;
            _rxStatus = RX_STATUS_ACK_CHECKSUM_FAILED;
            return false;
          }
        }
      } else {//Failed checksum, need to clear this out anyway.
        if (_debug) {
          if (_rxIdentifier != 0){//If we recieved a data packet.
            _debugPort->print(F("DBG:rx data packet checksum:"));_debugPort->println(rxBuffer[0]);
            _debugPort->println(F("DBG:ERROR, rx data packet checksum FAILED."));
            _rxStatus = RX_STATUS_DATA_CHECKSUM_FAILED;
          } else {//We recieved an ack packet.
            _debugPort->print(F("DBG:rx ack packet checksum:"));_debugPort->println(rxBuffer[0]);
            _debugPort->println(F("DBG:ERROR, rx ack packet checksum FAILED."));
            _rxStatus = RX_STATUS_ACK_CHECKSUM_FAILED;
          }
        }
        _rxLength = 0;//Reset so we can recieve next packet.
        _rxArrayInx = 0;
        return false;
      }
    } else {//Did not get the whole packet.
      _rxStatus = RX_STATUS_UNKNOWN;
      return false;
    }
  }
  _rxStatus = RX_STATUS_UNKNOWN;
  return false;
}
