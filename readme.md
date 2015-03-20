Arduino based kiln (or any temp control system) controller. Uses Adafruit's MAX31855 thermocouple amp breakout board (http://www.adafruit.com/products/269) and Brett Beauregard's excellent PID code (http://playground.arduino.cc/Code/PIDLibrary).

Currently runs an old Duncan 1027 240v kiln. Uses 2 40A SSRs and works pretty well. I'll post a wiring diagram soon.

Right now the ramp stages are hard coded but the timing logic seems to work. I will do a test fire this weekend.

To do:
Add ability to load a firing plan live or from an SD card
Add LCD display to allow plan selection and display status
Add data logging to record each firing for reference
Add alarms and info messages using local interface and/or network

