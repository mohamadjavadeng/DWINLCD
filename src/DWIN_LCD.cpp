#include "DWIN_LCD.h"

DWIN_LCD::DWIN_LCD(HardwareSerial& serial) : _serial(serial), _readIndex(0), _expectedLength(0) {}

void DWIN_LCD::begin(long int baudrate){
    _serial.begin(baudrate);
}

bool DWIN_LCD::isConnected(void){
    byte command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x31, 0x01};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return false;
    }
    bool result = (_response[0] == 0x5A && _response[1] == 0xA5 &&
                   _response[2] == 0x06 && _response[3] == 0x83 &&
                   _response[4] == 0x00 && _response[5] == 0x31);
    memset(_response, 0, sizeof(_response));
    return result;
}

void DWIN_LCD::nextPage(){
    byte currentPage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    _resetParser();
    _serial.write(currentPage, sizeof(currentPage));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    if (_response[8] == 0xFF){
        ++_response[7];
        _response[8] = 0x00;
    }
    else{
       ++_response[8];
    }
    byte page1 = _response[8];
    byte page2 = _response[7];
    byte nextpage[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, page2, page1};
    _resetParser();
    _serial.write(nextpage, sizeof(nextpage));
    start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::previousPage(){
    byte currentPage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    _resetParser();
    _serial.write(currentPage, sizeof(currentPage));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    if (_response[8] == 0x00){
        --_response[7];
        _response[8] = 0xFF;
    }
    else{
       --_response[8];
    }
    byte page1 = _response[8];
    byte page2 = _response[7];
    byte previouspage[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, page2, page1};
    _resetParser();
    _serial.write(previouspage, sizeof(previouspage));
    start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::gotoPage(const byte page){
    byte command[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, 0x00, page};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::writeSingleReg(uint16_t registeraddress, const uint16_t value){
    uint8_t highByte = (registeraddress >> 8) & 0xFF;
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t highValue = (value >> 8) & 0xFF;
    uint8_t lowValue = value & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x05, 0x82, highByte, lowByte, highValue, lowValue};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::writeData(uint16_t registeraddress, const uint8_t data[], const uint8_t length){
    uint8_t highByte = (registeraddress >> 8) & 0xFF;
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t newData[64];
    newData[0] = 0x5A;
    newData[1] = 0xA5;
    newData[2] = length + 3;
    newData[3] = 0x82;
    newData[4] = highByte;
    newData[5] = lowByte;
    for(int i = 0; i < length; i++){
        newData[i+6] = data[i];
    }
    _resetParser();
    _serial.write(newData, length + 6);
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::setSingleBit(uint16_t registeraddress, uint16_t Bitnumber){
    int val = 1;
    val = val << Bitnumber;
    uint16_t readPv = readSingleReg(registeraddress);
    readPv |= val;
    writeSingleReg(registeraddress, readPv);
}

void DWIN_LCD::resetSingleBit(uint16_t registeraddress, uint16_t Bitnumber){
    int val = 1;
    val = val << Bitnumber;
    uint16_t readPv = readSingleReg(registeraddress);
    readPv &= ~val;
    writeSingleReg(registeraddress, readPv);
}

void DWIN_LCD::readReg(uint16_t registeraddress, byte Nregisters, uint8_t* data){
    uint8_t highByte = (registeraddress >> 8) & 0xFF;
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, Nregisters};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    for(int i = 0; i < 2 * Nregisters; i++){
        data[i] = _response[i + 7];
    }
    memset(_response, 0, sizeof(_response));
}

uint16_t DWIN_LCD::readSingleReg(const uint16_t registeraddress){
    uint8_t highByte = (registeraddress >> 8) & 0xFF;
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, 0x01};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return 0;
    }
    uint16_t highRes = _response[7];
    uint16_t result = (highRes << 8) + _response[8];
    memset(_response, 0, sizeof(_response));
    return result;
}

bool DWIN_LCD::readSingleBit(const uint16_t registeraddress, const uint16_t Bitnumber){
    uint8_t highByte = (registeraddress >> 8) & 0xFF;
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, 0x01};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return false;
    }
    uint16_t highRes = _response[7];
    uint16_t result = (highRes << 8) + _response[8];
    uint16_t bitIndex = (1 << Bitnumber) & result;
    memset(_response, 0, sizeof(_response));
    return bitIndex != 0;
}

void DWIN_LCD::readRTC(void){
    byte command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x10, 0x04};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    year = _response[7];
    month = _response[8];
    day = _response[9];
    weekday = _response[10];
    hour = _response[11];
    minute = _response[12];
    second = _response[13];
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::writeRTC(uint8_t dayrtc, uint8_t monthrtc, uint8_t yearrtc, uint8_t hourrtc, uint8_t minutertc, uint8_t secondrtc, weekdays weekdayrtc){
    byte command[] = {0x5A, 0xA5, 0x0B, 0x82, 0x00, 0x10, yearrtc, monthrtc, dayrtc, weekdayrtc, hourrtc, minutertc, secondrtc, 0x00};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::backlight(void){
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x31, 0x01};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    backlightValue = _response[7];
    backlightCurrent = _response[8];
    memset(_response, 0, sizeof(_response));
}

void DWIN_LCD::buzzer(buzzer_duration buzzer){
    byte duration;
    if(buzzer == BUZZ_1SEC){
        duration = 0x7D;
    }
    else if(buzzer == BUZZ_500MSEC){
        duration = 0x3E;
    }
    else if(buzzer == BUZZ_250MSEC){
        duration = 0x20;
    }
    else{
        duration = 0x7D;
    }
    byte command[] = {0x5A, 0xA5, 0x05, 0x82, 0x00, 0xA0, 0x00, duration};
    _resetParser();
    _serial.write(command, sizeof(command));
    unsigned long start = millis();
    while (!_readResponse()) {
        if (millis() - start > 300) return;
    }
    memset(_response, 0, sizeof(_response));
}

bool DWIN_LCD::readResponse(void) {
    return _readResponse();
}

void DWIN_LCD::_resetParser(void) {
    _readIndex = 0;
    _expectedLength = 0;
}

bool DWIN_LCD::_readResponse(void) {
  while (_serial.available()) {
    uint8_t b = _serial.read();

    if (_readIndex == 0 && b != 0x5A) continue;
    if (_readIndex == 1 && b != 0xA5) {
      _readIndex = 0;
      continue;
    }

    _response[_readIndex++] = b;

    if (_readIndex == 3) {
      _expectedLength = _response[2] + 3;
    }

    if (_expectedLength && _readIndex >= _expectedLength) {
      _readIndex = 0;
      _expectedLength = 0;
      return true;
    }

    if (_readIndex >= sizeof(_response)) {
      _readIndex = 0;
      _expectedLength = 0;
    }
  }
  return false;
}
