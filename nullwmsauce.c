#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>

static const char *term[] = { "x-terminal-emulator", "-geometry", "169x54", "-e", "tmux", NULL };
static const char *emacs[] = { "emacs23", "-mm", NULL };

void spawn(Display * disp, const char** com)
{
    if (fork()) return;
    if (disp) close(ConnectionNumber(disp));
    setsid();
    execvp((char*)com[0], (char**)com);
}

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
            spawn(disp, term);
        if(ev.type == KeyPress &&(XLookupKeysym(&ev.xkey, 0) == XK_F10))
            spawn(disp, emacs);
    }
}
