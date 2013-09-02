#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void)
{
    Display * disp;
    XEvent ev;

    if(!(disp = XOpenDisplay(0x0))) return 1;
    XGrabKey(disp, XKeysymToKeycode(disp, XK_F11), Mod1Mask,
            DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp, XK_F10), Mod1Mask,
            DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
    for(;;){
        XNextEvent(disp, &ev);
        if(ev.type == KeyPress &&(XLookupKeysym(&ev.xkey, 0) == XK_F11))
		system("exec xterm -maximized");
        if(ev.type == KeyPress &&(XLookupKeysym(&ev.xkey, 0) == XK_F10))
		system("exec emacs -mm");
    }
}
