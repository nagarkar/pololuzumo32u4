<b>This is the Blinky example for the Pololu Zumo 32U4, which blinks the on-board Red LED once per second.

The example uses the QP framework, which requires a tick timer to drive scheduling of state machine execution. Timer 4 is used for this purpose, which is also used by the Pololu buzzer (https://goo.gl/MKkxna)

It also displays the number of milliseconds elapsed since the last time the LED was switched, using the LCD display. This number should be about 1000 ms = 1 sec.

<b>The example demonstrates:

1. One active object class "Sumo" (inside the package "AOs")
2. A simple state machine
