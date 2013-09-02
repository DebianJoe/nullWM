#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void)
{
    Display * disp;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if(!(disp = XOpenDisplay(0x0))) return 1;

    XGrabKey(disp, XKeysymToKeycode(disp, XK_F11), Mod1Mask,
            DefaultRootWindow(disp), True, GrabModeAsync, GrabModeAsync);
    XGrabButton(disp, 1, Mod1Mask, DefaultRootWindow(disp), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(disp, 3, Mod1Mask, DefaultRootWindow(disp), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;
    for(;;)
    {
        XNextEvent(disp, &ev);
        if(ev.type == KeyPress &&(XLookupKeysym(&ev.xkey, 0) == XK_F11))
		system("exec xterm");
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None){
	    XRaiseWindow(disp, ev.xkey.subwindow);
            XGetWindowAttributes(disp, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        else if(ev.type == MotionNotify && start.subwindow != None){
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(disp, start.subwindow,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease){
            start.subwindow = None;
        }
    }
}
