/* nullWM is a very simple stacking window manager
 * inspired by tinyWM and written by Joe Brock
 * <DebianJoe@Linuxbbq.org> */

/*NOT DONE, DON'T USE YET!!!!*/
#include <X11/Xlib.h>
#include <X11/keysym.h>

Display *disp;
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main()
{
    //Display * disp;
    Window root;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if(!(disp = XOpenDisplay(0x0))) return 1;

    root = DefaultRootWindow(disp);

    XGrabKey(disp, XKeysymToKeycode(disp, XK_Shift_L), Mod1Mask, root,
	     True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp, XK_Return), Mod1Mask, root,
	     True, GrabModeAsync, GrabModeAsync);
    XGrabButton(disp, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(disp, 3, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);

    for(;;){
        XNextEvent(disp, &ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None)
            XRaiseWindow(disp, ev.xkey.subwindow);
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None){
            XGrabPointer(disp, ev.xbutton.subwindow, True,
                    PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
                    GrabModeAsync, None, None, CurrentTime);
            XGetWindowAttributes(disp, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        else if(ev.type == MotionNotify){
            int xdiff, ydiff;
            while(XCheckTypedEvent(disp, MotionNotify, &ev));
            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(disp, ev.xmotion.window,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
            XUngrabPointer(disp, CurrentTime);
    }
}
