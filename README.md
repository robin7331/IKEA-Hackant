# Changes in this fork
- Added filter for first reported position above max limit. 
  - This prevents the table from moving to lowest position when plugging everything in.
- Some code cleanup
- Added 3D Printed case, see below

## 3D printed case on Thingiverse
https://www.thingiverse.com/thing:5181601

![Case3](Case3.jpeg)

The case contains 4 buttons. Two are for manually raising and lowering the desk. 
The other two move the desk to fixed positions (sitting / standing).

The PCB sandwich consists of the original IKEA board, Arduino Nano 3, a custom board containing the other components.
Lastly the black box is a DC/DC converter. Converting the desk power (24V) to 5V for the Arduino.


# Original readme starts here

# IKEA Hackant

A simple LIN slave attached to the stock controller board adds two memory buttons.
If you are interested in how this was developed check out my YouTube Tutorial

[![YouTube Tutorial](http://img.youtube.com/vi/AB75AxprXqQ/0.jpg)](https://www.youtube.com/watch?v=AB75AxprXqQ "IKEA Bekant Table Hacking")

## Schematics

Simple schematics created with [Fritzing](http://fritzing.org/home/ "Fritzing").

![Schematics](Schematics_schem.png)
![Board](Board.png)
