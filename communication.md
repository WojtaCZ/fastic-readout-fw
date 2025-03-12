
# Device communication protocol

## General device commands
### STATUS [get]

Power byte: all PGs and enable
Clock byte: Enable, reset, LOL, LOS
USB byte: Reserved for USB CC 
3V3 voltage
temperature

### ID [get]
STM UID
FW verze (hash)


## HV power supply commands
### ENABLE [get/set]
### CURRENT [get]
### VOLTAGE [get/set]

## Fastic Commands

### REGISTER [get/set]
### VOLTAGE [get]
### SYNCRESET [get/set]
### CALPULSE [get/set]
### TIME [get]
### AURORA [get/set]

Syntax Examples:
* get fastic register 0x00
* set fastic register 0x00 0x7f


### Userboard commands
### STATUS [get]
I2C enabled:
    ID, name[64], HV voltage
else:
    ID pins

### REGISTER [get/set]

### ID [get/set]

### NAME [get/set]

### VOLTAGE [get/set]

### STORE
Ulozi aktualni konfig vseho do eeprom

### LOAD 
Nacte konfig z eeprom do readoutu
