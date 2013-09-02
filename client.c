/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2011 Ciaran Anscomb <evilwm@6809.org.uk>
 * see README for license and other details. */

#include <stdio.h>
#include <stdlib.h>
#include "evilwm.h"
#include "log.h"

#define MAXIMUM_PROPERTY_LENGTH 4096

static int send_xmessage(Window w, Atom a, long x);

/* used all over the place.  return the client that has specified window as
 * either window or parent */

Client *find_client(Window w) {
	struct list *iter;

	for (iter = clients_tab_order; iter; iter = iter->next) {
		Client *c = iter->data;
		if (w == c->parent || w == c->window)
			return c;
	}
	return NULL;
}

void client_hide(Client *c) {
	c->ignore_unmap++;  /* Ignore unmap so we don't remove client */
	XUnmapWindow(dpy, c->parent);
	set_wm_state(c, IconicState);
}

void client_show(Client *c) {
	XMapWindow(dpy, c->parent);
	set_wm_state(c, NormalState);
}

void client_raise(Client *c) {
	XRaiseWindow(dpy, c->parent);
	clients_stacking_order = list_to_tail(clients_stacking_order, c);
	ewmh_set_net_client_list_stacking(c->screen);
}

/* This doesn't just call XLowerWindow(), as that could push the window
 * below "DESKTOP" type windows we're not managing. */
void client_lower(Client *c) {
	struct list *iter;
	Client *below;
	Window order[2];
	/* Find lowest other client in stacking order that is visible on the
	 * same screen. */
	for (iter = clients_stacking_order; iter; iter = iter->next) {
		below = iter->data;
		if (below == c)
			return;
		if (below->screen == c->screen && (is_fixed(below) || below->vdesk == c->phy->vdesk))
			break;
	}
	if (!iter) return;
	order[0] = below->parent;
	order[1] = c->parent;
	XRestackWindows(dpy, order, 2);
	clients_stacking_order = list_delete(clients_stacking_order, c);
	clients_stacking_order = list_insert_before(clients_stacking_order, iter, c);
	ewmh_set_net_client_list_stacking(c->screen);
}

/** client_calc_cog:
 *   Calculate the centre of gravity for a particular client
 */
void client_calc_cog(Client *c) {
	c->cog.x = c->width/2;
	c->cog.y = c->height/2;
	/* xxx: handle shaped windows oneday */
}

/** client_update_phy:
 *   Update the client's notion of which physical screen it belongs.
 */
void client_calc_phy(Client *c) {
	client_update_screenpos(c, client_to_Xcoord(c,x), client_to_Xcoord(c,y));
	/* if the client changes physical screens, the vdesk changes too */
	if (c->vdesk != VDESK_FIXED && c->vdesk != c->phy->vdesk) {
		c->vdesk = c->phy->vdesk;
		ewmh_set_net_wm_desktop(c);
	}
}

/** client_update_screenpos:
 *   Update the client's position using screen coordinates and
 *   the client's notion of which physical screen it belongs.
 *  NB, this routine must be used when translating from X11 screen co-ordinates
 */
void client_update_screenpos(Client *c, int screen_x, int screen_y) {
	c->phy = find_physical_screen(c->screen, screen_x + c->cog.x, screen_y + c->cog.y);
	c->nx = screen_x - c->phy->xoff;
	c->ny = screen_y - c->phy->yoff;
}

void set_wm_state(Client *c, int state) {
	/* Using "long" for the type of "data" looks wrong, but the
	 * fine people in the X Consortium defined it this way
	 * (even on 64-bit machines).
	 */
	long data[2];
	data[0] = state;
	data[1] = None;
	XChangeProperty(dpy, c->window, xa_wm_state, xa_wm_state, 32,
			PropModeReplace, (unsigned char *)data, 2);
}

/* Inform the client of the current window configuration */
void send_config(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.event = c->window;
	ce.window = c->window;
	ce.x = client_to_Xcoord(c,x);
	ce.y = client_to_Xcoord(c,y);
	ce.width = c->width;
	ce.height = c->height;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;

	XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

/* Shift client to show border according to window's gravity. */
void gravitate_border(Client *c, int bw) {
	int dx = 0, dy = 0;
	switch (c->win_gravity) {
	default:
	case NorthWestGravity:
		dx = bw;
		dy = bw;
		break;
	case NorthGravity:
		dy = bw;
		break;
	case NorthEastGravity:
		dx = -bw;
		dy = bw;
		break;
	case EastGravity:
		dx = -bw;
		break;
	case CenterGravity:
		break;
	case WestGravity:
		dx = bw;
		break;
	case SouthWestGravity:
		dx = bw;
		dy = -bw;
		break;
	case SouthGravity:
		dy = -bw;
		break;
	case SouthEastGravity:
		dx = -bw;
		dy = -bw;
		break;
	}
	if (c->nx != 0 || c->width != c->phy->width) {
		c->nx += dx;
	}
	if (c->ny != 0 || c->height != c->phy->height) {
		c->ny += dy;
	}
	/* XXX: do we need to recalculate phy? */
}

void select_client(Client *c) {
	if (current)
		XSetWindowBorder(dpy, current->parent, current->screen->bg.pixel);
	if (c) {
		unsigned long bpixel;
		if (is_fixed(c))
			bpixel = c->screen->fc.pixel;
		else
			bpixel = c->screen->fg.pixel;
		XSetWindowBorder(dpy, c->parent, bpixel);
		XInstallColormap(dpy, c->cmap);
		XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
	}
	current = c;
	ewmh_set_net_active_window(c);
}

/** client_to_vdesk:
 *   Send a client to a particular vdesk, mapping/unmapping it as required.
 */
void client_to_vdesk(Client *c, unsigned int vdesk) {
	if (valid_vdesk(vdesk)) {
		c->vdesk = vdesk;
		if (is_fixed(c) || c->vdesk == c->phy->vdesk) {
			client_show(c);
		} else {
			client_hide(c);
		}
		ewmh_set_net_wm_desktop(c);
		select_client(current);
	}
}

void remove_client(Client *c) {
	LOG_ENTER("remove_client(window=%lx, %s)", c->window, c->remove ? "withdrawing" : "wm quitting");

	XGrabServer(dpy);
	ignore_xerror = 1;

	/* ICCCM 4.1.3.1
	 * "When the window is withdrawn, the window manager will either
	 *  change the state field's value to WithdrawnState or it will
	 *  remove the WM_STATE property entirely."
	 * EWMH 1.3
	 * "The Window Manager should remove the property whenever a
	 *  window is withdrawn but it should leave the property in
	 *  place when it is shutting down." (both _NET_WM_DESKTOP and
	 *  _NET_WM_STATE) */
	if (c->remove) {
		LOG_DEBUG("setting WithdrawnState\n");
		set_wm_state(c, WithdrawnState);
		ewmh_withdraw_client(c);
	} else {
		ewmh_deinit_client(c);
	}

	gravitate_border(c, -c->border);
	gravitate_border(c, c->old_border);
	c->nx -= c->old_border;
	c->ny -= c->old_border;
	XReparentWindow(dpy, c->window, c->screen->root,
	                client_to_Xcoord(c,x), client_to_Xcoord(c,y));
	XSetWindowBorderWidth(dpy, c->window, c->old_border);
	XRemoveFromSaveSet(dpy, c->window);
	if (c->parent)
		XDestroyWindow(dpy, c->parent);

	clients_tab_order = list_delete(clients_tab_order, c);
	clients_mapping_order = list_delete(clients_mapping_order, c);
	clients_stacking_order = list_delete(clients_stacking_order, c);
	/* If the wm is quitting, we'll remove the client list properties
	 * soon enough, otherwise: */
	if (c->remove) {
		ewmh_set_net_client_list(c->screen);
		ewmh_set_net_client_list_stacking(c->screen);
	}

	if (current == c)
		current = NULL;  /* an enter event should set this up again */
	free(c);
#ifdef DEBUG
	{
		struct list *iter;
		int i = 0;
		for (iter = clients_tab_order; iter; iter = iter->next)
			i++;
		LOG_DEBUG("free(), window count now %d\n", i);
	}
#endif

	XUngrabServer(dpy);
	XSync(dpy, False);
	ignore_xerror = 0;
	LOG_LEAVE();
}

void send_wm_delete(Client *c, int kill_client) {
	int i, n, found = 0;
	Atom *protocols;

	if (!kill_client && XGetWMProtocols(dpy, c->window, &protocols, &n)) {
		for (i = 0; i < n; i++)
			if (protocols[i] == xa_wm_delete)
				found++;
		XFree(protocols);
	}
	if (found)
		send_xmessage(c->window, xa_wm_protos, xa_wm_delete);
	else
		XKillClient(dpy, c->window);
}

static int send_xmessage(Window w, Atom a, long x) {
	XEvent ev;

	ev.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = x;
	ev.xclient.data.l[1] = CurrentTime;

	return XSendEvent(dpy, w, False, NoEventMask, &ev);
}

#ifdef SHAPE
void set_shape(Client *c) {
	int bounding_shaped;
	int i, b;  unsigned int u;  /* dummies */

	if (!have_shape) return;
	/* Logic to decide if we have a shaped window cribbed from fvwm-2.5.10.
	 * Previous method (more than one rectangle returned from
	 * XShapeGetRectangles) worked _most_ of the time. */
	if (XShapeQueryExtents(dpy, c->window, &bounding_shaped, &i, &i,
				&u, &u, &b, &i, &i, &u, &u) && bounding_shaped) {
		LOG_DEBUG("%d shape extents\n", bounding_shaped);
		XShapeCombineShape(dpy, c->parent, ShapeBounding, 0, 0,
				c->window, ShapeBounding, ShapeSet);
	}
}
#endif

void *get_property(Window w, Atom property, Atom req_type,
	           unsigned long *nitems_return) {
	Atom actual_type;
	int actual_format;
	unsigned long bytes_after;
	unsigned char *prop;
	if (XGetWindowProperty(dpy, w, property,
	                       0L, MAXIMUM_PROPERTY_LENGTH / 4, False,
	                       req_type, &actual_type, &actual_format,
	                       nitems_return, &bytes_after, &prop) == Success) {
		if (actual_type == req_type)
			return (void *)prop;
		XFree(prop);
	}
	return NULL;
}
