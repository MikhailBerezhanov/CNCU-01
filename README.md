# CNCU-01
### Ethernet-CAN converter board based on STM32F407

([Learn more about this project](https://habr.com/ru/users/ClownMik/posts/))

![3D_model](https://hsto.org/webt/9c/j9/x_/9cj9x_lrqkpkajyzd6irhhqkfmu.jpeg)

This is open-source project that includes:

* PCB-design files for _Altium Designer_
* Firmware source for _Keil uVision 5_ and _STM CubeMX_


Firmware is based on: 

* STM HAL driver 
* [LwIP stack](https://ru.wikipedia.org/wiki/LwIP) 
* [FreeRTOS](https://www.freertos.org/)


### Board features

Power: 

* miniUSB 5V
* External 9V...24V

The main functions of this hardware are:

- Ethernet 10\100 Mb
- CAN
- SW

Optional this board and firmware supports: 

- RTC
- RS232 \ RS485
- microSD
- USB serial (COM)
- SPI
- I2C
- UART
- FLASH 2MB (external)
- EEPROM 32KB
- GPIO
- 3 LEDs
- 1 Buzzer

