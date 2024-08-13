#include "DWIN_LCD.h"

/*
The system debugging UART2 mode is fixed to 8N1, and the baud rate can be set.
*/
DWIN_LCD::DWIN_LCD(HardwareSerial& serial) : _serial(serial){}

/*
this function is called to initialize the baud rate
*/
void DWIN_LCD::begin(long int baudrate){
    _serial.begin(baudrate);
}

/*
this function is called to check if the device is connected
*/
bool DWIN_LCD::isConnected(void){
    bool result = false;
    byte command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x31, 0x01};
    uint8_t index = 0;
    uint8_t timeout = millis();
    _serial.write(command, sizeof(command));
    while(_serial.available() == 0) {
        delay(50);
        if(millis() - timeout > 300){
            return false;
        }
    }
    uint8_t *response = new uint8_t[9];
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    if(response[0] != 0x5A)result = false;
    else if(response[1] != 0xA5)result = false;
    else if(response[2] != 0x06)result = false;
    else if(response[3] != 0x83)result = false;
    else if(response[4] != 0x00)result = false;
    else if(response[5] != 0x31)result = false;
    else result = true;
    
    delete[] response;

    return result;
}

/*
this function automatically goes to the next page defined in the LCD
*/
void DWIN_LCD::nextPage(){
    byte currentPage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    uint8_t *response = new uint8_t[9]; 
    byte page1, page2;
    uint8_t index = 0;
    _serial.write(currentPage, sizeof(currentPage));
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    if(response[8] == 0xFF){
        ++response[7];
        response[8] = 0x00; 
    }
    else{
       ++response[8]; 
    }
    page1 = (byte)response[8];
    page2 = (byte)response[7];
    byte nextpage[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, page2, page1};
    _serial.write(nextpage, sizeof(nextpage));
    delete[] response;
}

/*
this function automatically goes to the previous page defined in the LCD
*/
void DWIN_LCD::previousPage(){
    byte currentPage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    uint8_t *response = new uint8_t[9];
    uint8_t index = 0;
    byte page1, page2;
    _serial.write(currentPage, sizeof(currentPage));
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    if(response[8] == 0x00){
        --response[7];
        response[8] = 0xFF; 
    }
    else{
       --response[8]; 
    }
    page1 = (byte)response[8];
    page2 = (byte)response[7];
    byte previouspage[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, page2, page1};
    _serial.write(previouspage, sizeof(previouspage));
    delete[] response;
}

/*
this function is called to choose a custom page
*/
void DWIN_LCD::gotoPage(const byte page){
    byte command[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, 0x00, page};
    _serial.write(command, sizeof(command));
}

/*
0x82 is the command structure for writing to the register
0x83 is the command structure for reading from a register
*/
/*
this function is called to write a byte/word data on a register
*/
void DWIN_LCD::writeSingleReg(uint16_t registeraddress, const uint16_t value){
    uint8_t highByte = (registeraddress >> 8) & 0xFF; // Extract high byte
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t highValue = (value >> 8) & 0xFF; // Extract high byte
    uint8_t lowValue = value & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x05, 0x82, highByte, lowByte, highValue, lowValue};
    _serial.write(command, sizeof(command));
}

/*
this function is called to write a text/array data on a register
*/
void DWIN_LCD::writeData(uint16_t registeraddress, const uint8_t data[], const uint8_t length){
    uint8_t highByte = (registeraddress >> 8) & 0xFF; // Extract high byte
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t* newData = new uint8_t[length + 6];
    newData[0] = 0x5A;
    newData[1] = 0xA5;
    newData[2] = length + 3;
    newData[3] = 0x82;
    newData[4] = highByte;
    newData[5] = lowByte;   
    for(int i = 0; i < length; i++){
        newData[i+6] = data[i];
    }
    _serial.write(newData, length + 6);
    delete[] newData;
}

/*
this function is used for set a specific bit of a register
*/
void DWIN_LCD::setSingleBit(uint16_t registeraddress, uint16_t Bitnumber){
    int val = 1;
    val = val << Bitnumber;
    uint16_t readPv = readSingleReg(registeraddress);
    readPv |= val;
    writeSingleReg(registeraddress, readPv);
}

/*
this function is used for reset a specific bit of a register
*/
void DWIN_LCD::resetSingleBit(uint16_t registeraddress, uint16_t Bitnumber){
    int val = 1;
    val = val << Bitnumber;
    uint16_t readPv = readSingleReg(registeraddress);
    readPv &= ~val;
    writeSingleReg(registeraddress, readPv);
}

/*
first seven bytes are constant and they are the information of th Packet
{5A}{A5},{0C},{83},{20}{00},{04},{data}
{Header 2 bytes}, {byte number of response}, {command}, {address 2 bytes},{length of response}, {data n bytes}
*/

/*
this function is used for reading an array of bytes from a packet of a register
*/
void DWIN_LCD::readReg(uint16_t registeraddress, byte Nregisters, uint8_t* data){
    
    uint8_t highByte = (registeraddress >> 8) & 0xFF; // Extract high byte
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, Nregisters};
    uint8_t* response = new uint8_t[2 * Nregisters + 7];
    int index = 0;
    _serial.write(command, sizeof(command));
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    for(int i = 0; i < 2 * Nregisters; i++){
        data[i] = response[i + 7];
    }
    delete[] response;

}

/*
this function is called when you want to read a byte/word of a register 
*/
uint16_t DWIN_LCD::readSingleReg(const uint16_t registeraddress){
    uint8_t highByte = (registeraddress >> 8) & 0xFF; // Extract high byte
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, 0x01};
    uint8_t *response = new uint8_t[9]; 
    _serial.write(command, sizeof(command));
    int index = 0;
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    uint16_t highRes = response[7];
    uint16_t result = (highRes << 8) + response[8];
    delete[] response;
    return result;
}

/*
for reading the status of a bit icon you can use the following function
*/
bool DWIN_LCD::readSingleBit(const uint16_t registeraddress, const uint16_t Bitnumber){
    uint8_t highByte = (registeraddress >> 8) & 0xFF; // Extract high byte
    uint8_t lowByte = registeraddress & 0xFF;
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, highByte, lowByte, 0x01};
    uint8_t *response = new uint8_t[9]; 
    _serial.write(command, sizeof(command));
    int index = 0;
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    uint16_t highRes = response[7];
    uint16_t result = (highRes << 8) + response[8];
    uint16_t bitIndex = (1 << Bitnumber) & result;
    delete[] response;
    if(bitIndex != 0) return true;
    return false;
}


/*
this function will read the internal RTC and store it in the variable pointed
*/
void DWIN_LCD::readRTC(void){
    byte command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x10, 0x04};
    uint8_t* response = new uint8_t[15];
    int index = 0;
    _serial.write(command, sizeof(command));
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    DWIN_LCD::year = response[7];
    DWIN_LCD::month = response[8];
    DWIN_LCD::day = response[9];
    DWIN_LCD::weekday = response[10];
    DWIN_LCD::hour = response[11];
    DWIN_LCD::minute = response[12];
    DWIN_LCD::second = response[13];
    delete[] response;
}

/*
internal RTC will be set using this function
*/
void DWIN_LCD::writeRTC(uint8_t dayrtc, uint8_t monthrtc, uint8_t yearrtc, uint8_t hourrtc, uint8_t minutertc, uint8_t secondrtc, weekdays weekdayrtc){
    byte command[] = {0x5A, 0xA5, 0x0B, 0x82, 0x00, 0x10, yearrtc, monthrtc, dayrtc, weekdayrtc, hourrtc, minutertc, secondrtc, 0x00};
    _serial.write(command, sizeof(command));
}

/*
read the backlight of LCD
*/
void DWIN_LCD::backlight(void){
    uint8_t command[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x31, 0x01};
    uint8_t *response = new uint8_t[9]; 
    _serial.write(command, sizeof(command));
    int index = 0;
    while(_serial.available() == 0) delay(50);
    while(_serial.available() != 0){
        response[index++] = _serial.read();
    }
    DWIN_LCD::backlightValue = (uint8_t)response[7];
    DWIN_LCD::backlightCurrent = (uint8_t)response[8];
    delete[] response;
}

/*
turn on buzzer of lcd for 1 second, 0.5 second , and 250 millisecond
*/
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
    _serial.write(command, sizeof(command));
}