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

This is not a regular joystick-controlled mouse. OR implements a special "rewind" operation
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

# Configuration and Building

Orbit Rat is configured in source code and built with [PlatformIO](https://platformio.org/).

The easiest way to use PlatformIO is just to use VisualStudio Code (not Visual Studio) with
the free PlatformIO plugin from the marketplace. Standalone setups may work but I haven't
tested them.

When you first build, it may seem to stall for a very long time. It's downloading a newer
compiler from a very slow connection. It gives no progress indicator. It seriously
took me 20 minutes to download. This only happens the first time you build.

Configuration is at the top of `main.cpp`.

# Hardware

This isn't commercial hardware or software. I assume you know your way around Arduino
and that you can follow instructions about installing and using Teensyduino
or PlatformIO (in vscode).

Assuming you'll use the software mostly unmodified, you need to hook up a pair of
potentiometer joysticks to the Teensy's analog inputs. That's pretty much it.

For the prototype I used the model sold on SparkFun, along with their breakout boards. 

I wired up the axes directly to the Teensy LC analog pins. I added pullup resistors on the switches
There are internal pullups that could work fine, but I didn't test that way.

You can calibrate the sticks by adjusting the `axisExtents` variable. There are also 
variables near that for controlling the motion speed.


# Software

The software isn't written to be especially efficient. It's written for a Teensy LC,
which has a 48MHz core clock. I haven't noticed that input rates are too low, so
I freely indulge in emulated floating point.

Calibration is quasi-automatic right now (although you can turn it off). By default, 
the center point is read at startup. Conservative endpoints are taken as a default,
but will expand if they receive a higher value from the hardware.

Configuration is kind of messy right now, via the `StickMode` struct and the
`modeMap` array. Button clicks cycle through the modes for each stick.

The configuration options are described in the code.

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