Examples for Pololu Zumo 32u4 robot running QP state machines.

You'll need a robot to run these, of course.

In addition, you'll need to install the QM modeler, and other prereqs (see Blinky Readme.md)

General Troubleshooting tips

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
