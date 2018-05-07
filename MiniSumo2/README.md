This is a simple state machine with two states:
- Paused: On entry, display a 'Press A' message on the LCD. Transition to GettingReady state on button press
- GettingReady: On entry, start a 5 second timer and countdown to zero on the LCD, with 100 ms increments. On 5 sec timer expiry, transition to Paused.

Prerequisites:
- See Prerequisites for Blinky Example.

<b>The example demonstrates:

1. In addition to what's in Blinky, this example demonstrates some interesting uses of the Timer


<b>Useful documentation
See Blinky Example, and code in the SumoProxmitySensors.ino example in the Pololu examples directory.