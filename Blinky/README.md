<b>This is the Blinky example for the Pololu Zumo 32U4, which blinks the on-board Red LED once per second.

Prerequisites:
- Install the QPN and Pololu zomo 32u4 libraries in Arduino
- Two good ways to do development on this:
-- Use Arduino IDE to compile and upload; Setup Arduino IDE with 'external editor' preference so you can use QM modeler for editing. Switch between them.
-- Use QM for everything, using external tools preferences as defined in the .Blinky file in the source directory.

The example uses the QP framework, which requires a tick timer to drive scheduling of state machine execution. Timer 4 is used for this purpose, which is also used by the Pololu buzzer (https://goo.gl/MKkxna)

It also displays the number of milliseconds elapsed since the last time the LED was switched, using the LCD display. This number should be about 1000 ms = 1 sec.

<b>The example demonstrates:

1. One active object class "Sumo" (inside the package "AOs")
2. A simple state machine


<b>Useful documentation
1. Shortform description of Timer 4 (https://goo.gl/9tUArV)
2. 32U4 processor datasheet: (https://goo.gl/4L7tAf)
3. Zumo 32u4 arduino library (https://www.pololu.com/docs/0J63/6)
4. Using QM Modeler for Atmega328/Uno (https://goo.gl/VZYq4W)
