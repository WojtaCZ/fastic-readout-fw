
# Device communication protocol

## General device commands
### STATUS [get]

Power byte: all PGs and enable
Clock byte: Enable, reset, LOL, LOS
USB byte: Reserved for USB CC 
3V3 voltage
temperature

| B0 | B1|
| --| --|
### ID [get]

| Bit [0:95] (96 bit) | Bit [96:151] (56 bit) | Bit [152:279] (128 bit) | Bit [280:311] (32 bit)
| --| --| -- | -- |
| UID | Git short commit | Git branch (first 16 characters) | Git build timestamp |

## HV power supply commands
### ENABLE [get/set]
### CURRENT [get]
| Bit [0:31] (32bit)|
| -- |
| HV current [uA] in floating point format |

### VOLTAGE [get/set]

| Bit [0:31] (32bit)|
| -- |
| HV voltage [V] in floating point format |

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
