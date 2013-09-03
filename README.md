nullWM
======
The anti-X11, X11 window manager.

Nullwm is simple.  It has exactly 2 functions.  Alt+F11 opens a Full-screen Xterm, and Alt+F10 Opens a Maximized Emacs session.  I might add an option to kill the wm later if I feel like it.

That's it.  That's all it does.  You can't move windows, or minimize, or chage desktops, or tile without a multiplexer, or....whatever other crap you'd normally do with X-based window managers.  It won't get along with your pagers, or any thing like that.

Still, it's all of 20 sloc of beautifully constructed C, and relies only on X11 headers.  It might be the smallest WM ever written for X.

Then What DOES it do?
======
It opens an Xterm.  
It open emacs.  

Why would I use that?
======
Because it's like having a no-X system, but you can still launch your silly X-based gtk or qt programs with all of their bloat being unobstructed by the bloat of your wm.  You can put a conky session in the background.  You can use whatever webbrowser you want to use.  You can set it up with xbindkeys, to launch your movie player.

Why did you do this?
========
This was for a short time a stackingWM with silly keybinds and all that, but then I remembered that I hate Xlib programming.  So, this is what I decided on.

How to Install.
========
For a standard install, cd to the directory of the source files and "make" and then with root permissions, "make install".  If you're running LinuxBBQ Sauce, instead of make, use "make sauce".


License
========
Copyright 2013 by Joe Brock <DebianJoe@linuxbbq.org>  

This program is released under the GPLv2.  

This README is more bloated than the entire window manager.  