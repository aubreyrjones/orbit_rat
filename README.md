Orbit Rat
---------
![Orbit Rat prototype](sticks.jpeg)

THIS IS NOT A MOUSE REPLACEMENT. The Orbit Rat is a Teensy LC based PC input device designed
to facilitate panning and orbiting in CAD and graphic design software.

The concept is similar to the 3dconnexion navigation device. But the Orbit Rat has fewer 
degrees of freedom, much wider compatibility, and much lower cost. The Orbit Rat is not 
quite a space mouse, you see.

The OR is designed around a pair of small joysticks, like those on a gamepad controller. These 
joysticks then control pan and orbit in CAD, art, and graphic design programs by specially
emulating a mouse. _No special drivers or plugins needed_, works in any operating system.

This is not a regular joystick-controlled mouse. This implements a special "rewind" operation
after every motion, which resets the mouse cursor position and unlocks near-infinite pan/orbit 
in any program that uses middle-click and drag for pan.

The cost to build one is around $30 USD. You could build it without any soldering, using
breadboard, but it will be bulky. You can easily solder it on protoboard with hand wiring.
It's also an easy circuit board to design.

Tested on Linux and Windows. Should work on Mac and mobile OS as well.

Tested to work with default settings in:

* Fusion 360
* Inkscape
* GIMP
* EAGLE CAD


# How it Works

When you tilt the stick outside the deadzone, Orbit Rat emulates a mouse holding down the middle
mouse button and starting a drag motion. This starts a pan action in lots of CAD and art software. 
The viewport will move as if you're panning with the mouse so long as you're tiling the stick.

If you think about how your favorite software works, when you finish the pan motion and let go the button,
the mouse is in a different part of the screen than when you started. You have to 
move the cursor back toward the middle of the screen to get more space if you want to continue the pan. 
With a traditional joystick-controlled mouse, this is torture.

But not so with Orbit Rat.

As you move, the OR tracks how much mouse motion it has sent to your computer. When you let
go of the stick, it very quickly moves the cursor back to where it was when you first tilted the stick. 
You can now continue the pan just by tilting the stick again. Or, you can continue your work 
with the cursor just where you left it.

The other stick works very similarly, but also emulates holding down the left shift keyboard button. 
This will activate an orbit in CAD software.

This is almost as much control as a dedicated "3d mouse" with special driver support. The beauty of 
Orbit Rat is that it works everywhere, in any operating system, in any application that uses middle 
mouse for pan.

The only requirement is that for the Teensy mouse specifically, you must set the acceleration to 0 in your
system settings. If you don't, the cursor will not return to the correct location. You should be able
to leave acceleration on for your regular mouse however you like it.


# Hardware

This isn't commercial hardware or software. I assume you know your way around Arduino
and that you can follow instructions about installing and using Teensyduino
or PlatformIO (in vscode).

Assuming you'l use the software mostly unmodified, you need to hook up a pair of
potentiometer joysticks to the Teensy's analog inputs. That's pretty much it.

For the prototype I used the model sold on SparkFun, along with their breakout boards. 

I wired up the axes directly to the Teensy LC analog pins. I added pullup resistors on the switches, 
but the code doesn't currently use (or even read) the switches. And there are internal
pullups that should be fine.

You can calibrate the sticks by adjusting the `axisExtents` variable. There are also 
variables near that for controlling the motion speed.


# Software

The software isn't written to be especially efficient. It's written for a Teensy LC,
which has a 48MHz core clock. So I indulge in some emulated floating point.

Aside from startup center-point reads, calibration is manual right now, which is less than ideal. 
A calibration mode would be easy to add, but I don't need it yet.

Likewise there's not much abstraction for additional controls or axes. There should be
mode switching support, but I haven't implemented it yet.


## This is not a commercial venture, and the Orbit Rat is not for sale

The whole point of the project is that it's very, very cheap compared to the pro-grade
alternative. Stocking the parts, assembling them, and shipping them takes time
I have no interest in spending. And people have expectations of commercial products
(warranties, returns, etc.) that I don't want to deal with. I have a job, and a
few bucks just isn't worth the hassle.

To make it worth it to me, each unit would need to sell for $100 or more. And
that kind of defeats the purpose.

On the other hand, this is also an absolutely _ideal_ first microcontroller project.
It is within your abilities to build one. I believe in you.

(In the future, I may consider selling parts or a kit. But even then I don't 
anticipate selling completed units.)