#include "tpms.h"

// FUNCTIONS 
String retmanData(String txt, int shift) {
  // Return only manufacturer data string
  int start=txt.indexOf("data: ")+6+shift;
  return txt.substring(start,start+(36-shift));  
}

byte retByte(String Data,int start) {
  // Return a single byte from string
  int sp=(start)*2;
  char *ptr;
  return strtoul(Data.substring(sp,sp+2).c_str(),&ptr, 16);
}

long returnData(String Data,int start) {
  // Return a long value with little endian conversion
  return retByte(Data,start)|retByte(Data,start+1)<<8|retByte(Data,start+2)<<16|retByte(Data,start+3)<<24;
}

int returnBatt(String Data) {
  // Return battery percentage
  return retByte(Data,16);
}

int returnAlarm(String Data) {
  // Return battery percentage
  return retByte(Data,17);
}