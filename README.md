Examples for Pololu Zumo 32u4 robot running QP state machines.

Prerequisites:
- The Pololu Zumo 32u4 robot to run these examples.
- Install the QM modeler
- Arduino IDE
- Install the QPN and Pololu zomo 32u4 libraries in Arduino
- There are two good ways to do development on this:
-- Use Arduino IDE to compile and upload; Setup Arduino IDE with 'external editor' preference so you can use QM modeler for editing. Switch between them (QM for code, Arduino for compile/upload).
-- Use QM for everything, using external tools preferences as defined in the .Blinky file in the source directory.

Useful documentation:
1. Shortform description of Timer 4 (https://goo.gl/9tUArV)
2. 32U4 processor datasheet: (https://goo.gl/4L7tAf)
3. Zumo 32u4 arduino library (https://www.pololu.com/docs/0J63/6)
4. Using QM Modeler for Atmega328/Uno (https://goo.gl/VZYq4W)


General Troubleshooting tips:

Q. What to do when Arduino can't upload
A. What works for me is to press the reset button on the pololu two times quickly when the 'Upload' attempt is being made. 
In the verbose mode, this looks like a lot of lines printing out COM ports (searching for one that works). In Arduino IDE, you'll see the
message 'Uploading' in the header bar.

Q. How do I copy an example and use it as a starting point?
A.  Follow these steps:
1. Copy the directory containing qm file and other files. Rename to <newmodel>
2. Open the model (.qm file) in the <newmodel> dir.
3. Save as <newmodel>.qm
4. Change the name of the .ino file in QM interface, as <newmodel>.ino
5. In the <newmodel> directory, delete the old .qm and .ino files.
6. Open the <newmodel>.ino file in Arduino and launch (or launch it from QM depending on how you have things setup).
