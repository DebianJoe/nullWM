nullWM
======
The anti-X11, X11 window manager.

Nullwm is simple.  It has exactly 1 function.  Alt+F11 opens a Full-screen Xterm.  There is a secondary build that opens either urxvt(Alt+F11) or emacs(Alt+F10).  I might add an option to kill the wm later if I feel like it.

That's it.  That's all it does.  You can't move windows, or minimize, or change desktops, or tile without a multiplexer, or....whatever other crap you'd normally do with X-based window managers.  It won't get along with your pagers, or any thing like that.

Still, it's all of 20 sloc of beautifully constructed C, and relies only on X11 headers.  It might be the smallest WM ever written for X.

Then What DOES it do?
======
It opens a full-screen Xterm.    

If you use the "make sauce" option, it opens URXVT with F11, and Emacs with F10.

Why would I use that?
======
Because it's like having a no-X system, but you can still launch your silly X-based gtk or qt programs with all of their bloat being unobstructed by the bloat of your wm.  You can put a conky session in the background.  You can use whatever webbrowser you want to use.  You can set it up with xbindkeys, to launch your movie player.

Why did you do this?
========
This was for a short time a stackingWM with silly keybinds and all that, but then I remembered that I hate Xlib programming.  So, this is what I decided on.  I've spent a long time without using xserver at all, and honestly, it's often the difference between a "point-and-click" user, and someone who's going to learn how GNU / Linux really works, because by stripping away everything other than the bare essentials, it's easy to learn those in better detail.  

How to Install.
========
For a standard install, cd to the directory of the source files and "make" and then with root permissions, "make install".  If you're running LinuxBBQ Sauce, instead of make, use "make sauce".  I guess you could always use "make clean" too, if you hate bloat as much as I do.

Minor Issues?
========
Of course there are minor issues.  It's as minimalistic as a WM can get and still do something.  Also, if using the "Sauce64" version, urxvt geometry needs to be set in the code before you compile it.  Just find something that works for your display and hack it in there.  If you can't figure it out, then e-mail me.  After I make fun of you for a little while, I'll walk you through it.


License
========
Copyright 2013 by Joe Brock <DebianJoe@linuxbbq.org>  

This program is released under the GPLv2.  

This README is more bloated than the entire window manager.  
