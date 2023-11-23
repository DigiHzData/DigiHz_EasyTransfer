//Helper functions for printing 64 bit start.
//https://github.com/greiman/SdFat/issues/130
size_t printUint64_t(Print* pr, uint64_t n) {
  char buf[21];
  char *str = &buf[sizeof(buf) - 1];
  *str = '\0';
  do {
    uint64_t m = n;
    n /= 10;
    *--str = m - 10*n + '0';
  } while (n);
  pr->print(str);
}

size_t printInt64_t(Print* pr, int64_t n) {
  size_t s = 0;
  if (n < 0) {
    n = -n;
    s = pr->print('-');
  }  return s + printUint64_t(pr, (uint64_t)n);
}

size_t printlnUint64_t(Print* pr, uint64_t n) {
  return printUint64_t(pr, n) + pr->println();
}

size_t printlnInt64_t(Print* pr, int64_t n) {
  return printInt64_t(pr, n) + pr->println();
}
//Helper functions for printing 64 bit end.

//Helper functions for parsing recieved data start.
unsigned long processBytes2UnsignedLong(uint8_t * address, uint8_t startByte, bool bytes8 = false){
  if (!bytes8){
    uint8_t bytes2UnsignedLong[4];//Temporary array for making 4 bytes to a unsigned long.
    //Cast 4 bytes to a unsigned long
    bytes2UnsignedLong[0] = address[startByte];
    bytes2UnsignedLong[1] = address[startByte + 1];
    bytes2UnsignedLong[2] = address[startByte + 2];
    bytes2UnsignedLong[3] = address[startByte + 3];
    return *( (unsigned long*) bytes2UnsignedLong );//Pointer typecast method.
  } else {
    uint8_t bytes2UnsignedLong[8];//Temporary array for making 8 bytes to a unsigned long.
    //Cast 8 bytes to a unsigned long
    bytes2UnsignedLong[0] = address[startByte];
    bytes2UnsignedLong[1] = address[startByte + 1];
    bytes2UnsignedLong[2] = address[startByte + 2];
    bytes2UnsignedLong[3] = address[startByte + 3];
    bytes2UnsignedLong[4] = address[startByte + 4];
    bytes2UnsignedLong[5] = address[startByte + 5];
    bytes2UnsignedLong[6] = address[startByte + 6];
    bytes2UnsignedLong[7] = address[startByte + 7];
    return *( (unsigned long*) bytes2UnsignedLong );//Pointer typecast method.
  } 
}

long processBytes2Long(uint8_t * address, uint8_t startByte, bool bytes8 = false){
  if (!bytes8){
    uint8_t bytes2Long[4];//Temporary array for making 4 bytes to a long.
    //Cast 4 bytes to a long
    bytes2Long[0] = address[startByte];
    bytes2Long[1] = address[startByte + 1];
    bytes2Long[2] = address[startByte + 2];
    bytes2Long[3] = address[startByte + 3];
    return *( (long*) bytes2Long );//Pointer typecast method.
  } else {
    uint8_t bytes2Long[8];//Temporary array for making 8 bytes to a long.
    //Cast 8 bytes to a long
    bytes2Long[0] = address[startByte];
    bytes2Long[1] = address[startByte + 1];
    bytes2Long[2] = address[startByte + 2];
    bytes2Long[3] = address[startByte + 3];
    bytes2Long[4] = address[startByte + 4];
    bytes2Long[5] = address[startByte + 5];
    bytes2Long[6] = address[startByte + 6];
    bytes2Long[7] = address[startByte + 7];
    return *( (long*) bytes2Long );//Pointer typecast method.
  }
}

uint16_t processBytes2Uint16_t(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Uint16_t[2];//Temporary array for making 2 bytes to a uint16_t.
  //Cast 2 bytes to a uint16_t
  bytes2Uint16_t[0] = address[startByte];
  bytes2Uint16_t[1] = address[startByte + 1];
  return *( (uint16_t*) bytes2Uint16_t );//Pointer typecast method.
}

uint32_t processBytes2Uint32_t(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Uint32_t[4];//Temporary array for making 4 bytes to a uint32_t.
  //Cast 4 bytes to a uint32_t
  bytes2Uint32_t[0] = address[startByte];
  bytes2Uint32_t[1] = address[startByte + 1];
  bytes2Uint32_t[2] = address[startByte + 2];
  bytes2Uint32_t[3] = address[startByte + 3];
  return *( (uint32_t*) bytes2Uint32_t );//Pointer typecast method.
}

uint64_t processBytes2Uint64_t(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Uint64_t[8];//Temporary array for making 8 bytes to a uint64_t.
  //Cast 8 bytes to a uint64_t
  bytes2Uint64_t[0] = address[startByte];
  bytes2Uint64_t[1] = address[startByte + 1];
  bytes2Uint64_t[2] = address[startByte + 2];
  bytes2Uint64_t[3] = address[startByte + 3];
  bytes2Uint64_t[4] = address[startByte + 4];
  bytes2Uint64_t[5] = address[startByte + 5];
  bytes2Uint64_t[6] = address[startByte + 6];
  bytes2Uint64_t[7] = address[startByte + 7];
  return *( (uint64_t*) bytes2Uint64_t );//Pointer typecast method.
}

float processBytes2Float(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Float[4];//Temporary array for making 4 bytes to a float.
  //Cast 4 bytes to a float
  bytes2Float[0] = address[startByte];
  bytes2Float[1] = address[startByte + 1];
  bytes2Float[2] = address[startByte + 2];
  bytes2Float[3] = address[startByte + 3];
  return *( (float*) bytes2Float );//Pointer typecast method.
}

int8_t processByte2Int8_t(uint8_t * address, uint8_t startByte){
  int8_t byte2Int8_t[1];//Temporary array for making 1 byte to a int8_t.
  //Cast 1 byte to a int8_t
  byte2Int8_t[0] = address[startByte];
  return *( (int8_t*) byte2Int8_t );//Pointer typecast method.
}

int16_t processBytes2Int16_t(uint8_t * address, uint8_t startByte){
  int8_t bytes2Int16_t[2];//Temporary array for making 2 bytes to a int16_t.
  //Cast 2 bytes to a int16_t
  bytes2Int16_t[0] = address[startByte];
  bytes2Int16_t[1] = address[startByte + 1];
  return *( (int16_t*) bytes2Int16_t );//Pointer typecast method.
}

int32_t processBytes2Int32_t(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Int32_t[4];//Temporary array for making 4 bytes to a int32_t.
  //Cast 4 bytes to a int32_t
  bytes2Int32_t[0] = address[startByte];
  bytes2Int32_t[1] = address[startByte + 1];
  bytes2Int32_t[2] = address[startByte + 2];
  bytes2Int32_t[3] = address[startByte + 3];
  return *( (int32_t*) bytes2Int32_t );//Pointer typecast method.
}

int64_t processBytes2Int64_t(uint8_t * address, uint8_t startByte){
  uint8_t bytes2Int64_t[8];//Temporary array for making 8 bytes to a int64_t.
  //Cast 8 bytes to a int64_t
  bytes2Int64_t[0] = address[startByte];
  bytes2Int64_t[1] = address[startByte + 1];
  bytes2Int64_t[2] = address[startByte + 2];
  bytes2Int64_t[3] = address[startByte + 3];
  bytes2Int64_t[4] = address[startByte + 4];
  bytes2Int64_t[5] = address[startByte + 5];
  bytes2Int64_t[6] = address[startByte + 6];
  bytes2Int64_t[7] = address[startByte + 7];
  return *( (int64_t*) bytes2Int64_t );//Pointer typecast method.
}
//Helper functions for parsing recieved data end.
