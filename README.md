# cncu-01
Ethernet-CAN converter (Development board) based on STM32F407

This is open-source project that includes PCB-design files and part of firmware for CNCU-01 board.
PCB-design was developed using Altium Designer 17 and Firmware is based on STM HAL driver, LwIP stack and FreeRTOS and was developed with 'C' using Keil uVision 5 and STMCubeMX.

NOTE: Firmware is under development. Now it supports board's main modules features (HAL) + simple web interface but no application layer.

# Board supports:
- RTC
- RS232 \ RS485
- Ethernet 10\100 Mb
- microSD
- CAN
- USB serial (COM)
- SPI
- I2C
- UART
- FLASH 2MB (external)
- EEPROM 32KB
- SW \ JTAG
- GPIO
- 3 LEDs
- 1 Buzzer

Power: miniUSB 5V \ External 9..24V



