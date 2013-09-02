/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2011 Ciaran Anscomb <evilwm@6809.org.uk>
 * see README for license and other details. */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include "evilwm.h"
#include "log.h"

static int interruptibleXNextEvent(XEvent *event);

#ifdef DEBUG
const char *debug_atom_name(Atom a);
const char *debug_atom_name(Atom a) {
	static char buf[48];
	char *atom_name = XGetAtomName(dpy, a);
	strncpy(buf, atom_name, sizeof(buf));
	buf[sizeof(buf)-1] = 0;
	return buf;
}
#endif

static void handle_key_event(XKeyEvent *e) {
	KeySym key = XKeycodeToKeysym(dpy,e->keycode,0);
	Client *c;
	int width_inc, height_inc;
	ScreenInfo *current_screen;
	PhysicalScreen *current_phy;
	find_current_screen_and_phy(&current_screen, &current_phy);

	switch(key) {
		case KEY_NEW:
			spawn((const char *const *)opt_term);
			break;
		case KEY_NEXT:
			next();
			if (XGrabKeyboard(dpy, e->root, False, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess) {
				XEvent ev;
				do {
					XMaskEvent(dpy, KeyPressMask|KeyReleaseMask, &ev);
					if (ev.type == KeyPress && XKeycodeToKeysym(dpy,ev.xkey.keycode,0) == KEY_NEXT)
						next();
				} while (ev.type == KeyPress || XKeycodeToKeysym(dpy,ev.xkey.keycode,0) == KEY_NEXT);
				XUngrabKeyboard(dpy, CurrentTime);
			}
			ewmh_select_client(current);
			break;
		case KEY_DOCK_TOGGLE:
			set_docks_visible(current_screen, !current_screen->docks_visible);
			break;
		case XK_1: case XK_2: case XK_3: case XK_4:
		case XK_5: case XK_6: case XK_7: case XK_8:
		case XK_9: case XK_0:
			switch_vdesk(current_screen, current_phy, KEY_TO_VDESK(key));
			break;
		case KEY_PREVDESK:
			/* Find next avaliable vdesk */
			for (unsigned i = current_phy->vdesk - 1; i < opt_vdesks ; i--) {
				if (switch_vdesk(current_screen, current_phy, i))
					break;
			}
			break;
		case KEY_NEXTDESK:
			/* Find prev avaliable vdesk */
			for (unsigned i = current_phy->vdesk + 1; i < opt_vdesks; i++) {
				if (switch_vdesk(current_screen, current_phy, i))
					break;
			}
			break;
		case KEY_TOGGLEDESK:
			switch_vdesk(current_screen, current_phy, current_screen->old_vdesk);
			break;
		case KEY_EXGPHY:
			exchange_phy(current_screen);
			break;
		default: break;
	}
	c = current;
	if (c == NULL) return;
	width_inc = (c->width_inc > 1) ? c->width_inc : 16;
	height_inc = (c->height_inc > 1) ? c->height_inc : 16;
	switch (key) {
		case KEY_LEFT:
			if (e->state & altmask) {
				if ((c->width - width_inc) >= c->min_width)
					c->width -= width_inc;
			} else {
				c->nx -= 16;
			}
			goto move_client;
		case KEY_DOWN:
			if (e->state & altmask) {
				if (!c->max_height || (c->height + height_inc) <= c->max_height)
					c->height += height_inc;
			} else {
				c->ny += 16;
			}
			goto move_client;
		case KEY_UP:
			if (e->state & altmask) {
				if ((c->height - height_inc) >= c->min_height)
					c->height -= height_inc;
			} else {
				c->ny -= 16;
			}
			goto move_client;
		case KEY_RIGHT:
			if (e->state & altmask) {
				if (!c->max_width || (c->width + width_inc) <= c->max_width)
					c->width += width_inc;
			} else {
				c->nx += 16;
			}
			goto move_client;
		case KEY_TOPLEFT:
			c->nx = c->border;
			c->ny = c->border;
			goto move_client;
		case KEY_TOPRIGHT:
			c->nx = c->phy->width - c->width - c->border;
			c->ny = c->border;
			goto move_client;
		case KEY_BOTTOMLEFT:
			c->nx = c->border;
			c->ny = c->phy->height - c->height - c->border;
			goto move_client;
		case KEY_BOTTOMRIGHT:
			c->nx = c->phy->width - c->width - c->border;
			c->ny = c->phy->height - c->height - c->border;
			goto move_client;
		case KEY_LOWER: case KEY_ALTLOWER:
			client_lower(c);
			break;
		case KEY_INFO:
			show_info(c, e->keycode);
			break;
		case KEY_MAX:
			maximise_client(c, NET_WM_STATE_TOGGLE, MAXIMISE_HORZ|MAXIMISE_VERT);
			break;
		case KEY_MAXVERT:
			if (e->state & altmask) {
				maximise_client(c, NET_WM_STATE_TOGGLE, MAXIMISE_HORZ);
			} else {
				maximise_client(c, NET_WM_STATE_TOGGLE, MAXIMISE_VERT);
			}
			break;
		case KEY_FIX:
			if (is_fixed(c)) {
				client_to_vdesk(c, c->phy->vdesk);
			} else {
				client_to_vdesk(c, VDESK_FIXED);
			}
			break;
		default: break;
	}
	if (key == KEY_KILL)
		send_wm_delete(c, e->state & altmask);
	return;
move_client:
	if (abs(c->nx) == c->border && c->oldw != 0)
		c->nx = 0;
	if (abs(c->ny) == c->border && c->oldh != 0)
		c->ny = 0;
	client_calc_cog(c);
	client_calc_phy(c);
	moveresizeraise(c);
#ifdef WARP_POINTER
	setmouse(c->window, c->width + c->border - 1,
			c->height + c->border - 1);
#endif
	discard_enter_events(c);
	return;
}

static void handle_button_event(XButtonEvent *e) {
	Client *c = find_client(e->window);

	if (c) {
		switch (e->button) {
			case Button1:
				drag(c); break;
			case Button2:
				sweep(c); break;
			case Button3:
				client_lower(c); break;
			default: break;
		}
	}
}

static void do_window_changes(int value_mask, XWindowChanges *wc, Client *c,
		int gravity) {
	if (gravity == 0)
		gravity = c->win_gravity_hint;
	c->win_gravity = gravity;

	/* Warning: these co-ordinates are in screen co-ordinates */
	int screen_x = client_to_Xcoord(c,x);
	int screen_y = client_to_Xcoord(c,y);
	if (value_mask & CWX) screen_x = wc->x;
	if (value_mask & CWY) screen_y = wc->y;
	client_update_screenpos(c, screen_x, screen_y);

	if (value_mask & (CWWidth|CWHeight)) {
		int dw = 0, dh = 0;
		if (!(value_mask & (CWX|CWY))) {
			gravitate_border(c, -c->border);
		}
		if (value_mask & CWWidth) {
			int neww = wc->width;
			if (neww < c->min_width)
				neww = c->min_width;
			if (c->max_width && neww > c->max_width)
				neww = c->max_width;
			dw = neww - c->width;
			c->width = neww;
		}
		if (value_mask & CWHeight) {
			int newh = wc->height;
			if (newh < c->min_height)
				newh = c->min_height;
			if (c->max_height && newh > c->max_height)
				newh = c->max_height;
			dh = newh - c->height;
			c->height = newh;
		}
		/* only apply position fixes if not being explicitly moved */
		if (!(value_mask & (CWX|CWY))) {
			switch (gravity) {
			default:
			case NorthWestGravity:
				break;
			case NorthGravity:
				c->nx -= (dw / 2);
				break;
			case NorthEastGravity:
				c->nx -= dw;
				break;
			case WestGravity:
				c->ny -= (dh / 2);
				break;
			case CenterGravity:
				c->nx -= (dw / 2);
				c->ny -= (dh / 2);
				break;
			case EastGravity:
				c->nx -= dw;
				c->ny -= (dh / 2);
				break;
			case SouthWestGravity:
				c->ny -= dh;
				break;
			case SouthGravity:
				c->nx -= (dw / 2);
				c->ny -= dh;
				break;
			case SouthEastGravity:
				c->nx -= dw;
				c->ny -= dh;
				break;
			}
			value_mask |= CWX|CWY;
			gravitate_border(c, c->border);
		}
		/* Recalculate the centre of gravity if resized */
		/* This may change the display to which the client belongs */
		client_calc_cog(c);
		client_calc_phy(c);
	}
	wc->x = client_to_Xcoord(c,x) - c->border;
	wc->y = client_to_Xcoord(c,y) - c->border;
	wc->border_width = c->border;
	XConfigureWindow(dpy, c->parent, value_mask, wc);
	XMoveResizeWindow(dpy, c->window, 0, 0, c->width, c->height);
	if ((value_mask & (CWX|CWY)) && !(value_mask & (CWWidth|CWHeight))) {
		send_config(c);
	}
}

static void handle_configure_request(XConfigureRequestEvent *e) {
	Client *c = find_client(e->window);
	XWindowChanges wc;

	wc.x = e->x;
	wc.y = e->y;
	wc.width = e->width;
	wc.height = e->height;
	wc.border_width = 0;
	wc.sibling = e->above;
	wc.stack_mode = e->detail;
	if (c) {
		if (e->value_mask & CWStackMode && e->value_mask & CWSibling) {
			Client *sibling = find_client(e->above);
			if (sibling) {
				wc.sibling = sibling->parent;
			}
		}
		do_window_changes(e->value_mask, &wc, c, 0);
		if (c == current) {
			discard_enter_events(c);
		}
	} else {
		LOG_XENTER("XConfigureWindow(window=%lx, value_mask=%lx)", (unsigned int)e->window, e->value_mask);
		XConfigureWindow(dpy, e->window, e->value_mask, &wc);
		LOG_XLEAVE();
	}
}

static void handle_map_request(XMapRequestEvent *e) {
	Client *c = find_client(e->window);

	LOG_ENTER("handle_map_request(window=%lx)", e->window);
	if (c) {
		if (!is_fixed(c) && c->vdesk != c->phy->vdesk)
			switch_vdesk(c->screen, c->phy, c->vdesk);
		client_show(c);
		client_raise(c);
	} else {
		XWindowAttributes attr;
		XGetWindowAttributes(dpy, e->window, &attr);
		make_new_client(e->window, find_screen(attr.root));
	}
	LOG_LEAVE();
}

static void handle_unmap_event(XUnmapEvent *e) {
	Client *c = find_client(e->window);

	LOG_ENTER("handle_unmap_event(window=%lx)", e->window);
	if (c) {
		if (c->ignore_unmap) {
			c->ignore_unmap--;
			LOG_DEBUG("ignored (%d ignores remaining)\n", c->ignore_unmap);
		} else {
			LOG_DEBUG("flagging client for removal\n");
			c->remove = 1;
			need_client_tidy = 1;
		}
	} else {
		LOG_DEBUG("unknown client!\n");
	}
	LOG_LEAVE();
}

static void handle_colormap_change(XColormapEvent *e) {
	Client *c = find_client(e->window);

	if (c && e->new) {
		c->cmap = e->colormap;
		XInstallColormap(dpy, c->cmap);
	}
}

static void handle_property_change(XPropertyEvent *e) {
	Client *c = find_client(e->window);

	if (c) {
		LOG_ENTER("handle_property_change(window=%lx, atom=%s)", e->window, debug_atom_name(e->atom));
		if (e->atom == XA_WM_NORMAL_HINTS) {
			get_wm_normal_hints(c);
			LOG_DEBUG("geometry=%dx%d\n", c->width, c->height);
		} else if (e->atom == xa_net_wm_window_type) {
			get_window_type(c);
			if (!c->is_dock && (is_fixed(c) || (c->vdesk == c->phy->vdesk))) {
				client_show(c);
			}
		}
		LOG_LEAVE();
	}
}

static void handle_enter_event(XCrossingEvent *e) {
	Client *c;

	if ((c = find_client(e->window))) {
		if (!is_fixed(c) && c->vdesk != c->phy->vdesk)
			return;
		select_client(c);
		ewmh_select_client(c);
	}
}

static void handle_mappingnotify_event(XMappingEvent *e) {
	XRefreshKeyboardMapping(e);
	if (e->request == MappingKeyboard) {
		int i;
		for (i = 0; i < num_screens; i++) {
			grab_keys_for_screen(&screens[i]);
		}
	}
}

#ifdef SHAPE
static void handle_shape_event(XShapeEvent *e) {
	Client *c = find_client(e->window);
	if (c)
		set_shape(c);
}
#endif

#ifdef RANDR
static void handle_randr_event(XRRScreenChangeNotifyEvent *e) {
	XRRUpdateConfiguration((XEvent*)e);
	ScreenInfo *s = find_screen(e->root);

	PhysicalScreen *old_phys = s->physical;
	int old_num_phys = s->num_physical;
	probe_screen(s);

	/* set all phys on screen to have a bogus vdesk */
	for (int i = 0; i < s->num_physical; i++) {
		s->physical[i].vdesk = VDESK_NONE;
	}

	/* for each phy, map an old vesk to it */
	unsigned int save_old_vdesk = s->old_vdesk;
	unsigned int vdesks[opt_vdesks];
	for (unsigned int i = 0; i < opt_vdesks; i++) {
		vdesks[i] = i;
	}
	for (int i = 0; i < s->num_physical; i++) {
		unsigned int vdesk = VDESK_NONE;
		if (i < old_num_phys) {
			vdesk = old_phys[i].vdesk;
		} else {
			/* this is a new phy, pick a vdesk to show */
			for (unsigned int j = 0; j < opt_vdesks; j++) {
				if (vdesks[j] == VDESK_NONE)
					continue;
				vdesk = vdesks[j];
				break;
			}
		}
		vdesks[vdesk] = VDESK_NONE;
		switch_vdesk(s, &s->physical[i], vdesk);
	}
	/* unmap clients on vdesks nolonger atatched to a current phy */
	for (int i = s->num_physical; i < old_num_phys; i++) {
		switch_vdesk(s, &old_phys[i], VDESK_NONE);
	}
	/* xxx, is there a better way to do this?, ie, avoid switch_vdesk, iterate
	   through all clients, map phys to vdesks, then continue as long ago */
	s->old_vdesk = save_old_vdesk;
	ewmh_set_screen_workarea(s);
}
#endif

static void handle_client_message(XClientMessageEvent *e) {
	ScreenInfo *s = find_current_screen();
	Client *c;

	LOG_ENTER("handle_client_message(window=%lx, format=%d, type=%s)", e->window, e->format, debug_atom_name(e->message_type));

	if (e->message_type == xa_net_current_desktop) {
		switch_vdesk(s, s->physical, e->data.l[0]);
		LOG_LEAVE();
		return;
	}
	c = find_client(e->window);
	if (!c && e->message_type == xa_net_request_frame_extents) {
		ewmh_set_net_frame_extents(e->window);
		LOG_LEAVE();
		return;
	}
	if (!c) {
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_active_window) {
		/* Only do this if it came from direct user action */
		if (e->data.l[0] == 2) {
			if (c->screen == s)
				select_client(c);
		}
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_close_window) {
		/* Only do this if it came from direct user action */
		if (e->data.l[1] == 2) {
			send_wm_delete(c, 0);
		}
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_moveresize_window) {
		/* Only do this if it came from direct user action */
		int source_indication = (e->data.l[0] >> 12) & 3;
		if (source_indication == 2) {
			int value_mask = (e->data.l[0] >> 8) & 0x0f;
			int gravity = e->data.l[0] & 0xff;
			XWindowChanges wc;

			wc.x = e->data.l[1];
			wc.y = e->data.l[2];
			wc.width = e->data.l[3];
			wc.height = e->data.l[4];
			do_window_changes(value_mask, &wc, c, gravity);
		}
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_restack_window) {
		/* Only do this if it came from direct user action */
		if (e->data.l[0] == 2) {
			XWindowChanges wc;

			wc.sibling = e->data.l[1];
			wc.stack_mode = e->data.l[2];
			do_window_changes(CWSibling | CWStackMode, &wc, c, c->win_gravity);
		}
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_wm_desktop) {
		/* Only do this if it came from direct user action */
		if (e->data.l[1] == 2) {
			client_to_vdesk(c, e->data.l[0]);
		}
		LOG_LEAVE();
		return;
	}
	if (e->message_type == xa_net_wm_state) {
		int i, maximise_hv = 0;
		/* Message can contain up to two state changes: */
		for (i = 1; i <= 2; i++) {
			if ((Atom)e->data.l[i] == xa_net_wm_state_maximized_vert) {
				maximise_hv |= MAXIMISE_VERT;
			} else if ((Atom)e->data.l[i] == xa_net_wm_state_maximized_horz) {
				maximise_hv |= MAXIMISE_HORZ;
			} else if ((Atom)e->data.l[i] == xa_net_wm_state_fullscreen) {
				maximise_hv |= MAXIMISE_VERT|MAXIMISE_HORZ;
			}
		}
		if (maximise_hv) {
			maximise_client(c, e->data.l[0], maximise_hv);
		}
		LOG_LEAVE();
		return;
	}
	LOG_LEAVE();
}

void event_main_loop(void) {
	union {
		XEvent xevent;
#ifdef SHAPE
		XShapeEvent xshape;
#endif
#ifdef RANDR
		XRRScreenChangeNotifyEvent xrandr;
#endif
	} ev;
	/* main event loop here */
	while (!wm_exit) {
		if (interruptibleXNextEvent(&ev.xevent)) {
			switch (ev.xevent.type) {
			case KeyPress:
				handle_key_event(&ev.xevent.xkey); break;
			case ButtonPress:
				handle_button_event(&ev.xevent.xbutton); break;
			case ConfigureRequest:
				handle_configure_request(&ev.xevent.xconfigurerequest); break;
			case MapRequest:
				handle_map_request(&ev.xevent.xmaprequest); break;
			case ColormapNotify:
				handle_colormap_change(&ev.xevent.xcolormap); break;
			case EnterNotify:
				handle_enter_event(&ev.xevent.xcrossing); break;
			case PropertyNotify:
				handle_property_change(&ev.xevent.xproperty); break;
			case UnmapNotify:
				handle_unmap_event(&ev.xevent.xunmap); break;
			case MappingNotify:
				handle_mappingnotify_event(&ev.xevent.xmapping); break;
			case ClientMessage:
				handle_client_message(&ev.xevent.xclient); break;
			default:
#ifdef SHAPE
				if (have_shape && ev.xevent.type == shape_event) {
					handle_shape_event(&ev.xshape);
				}
#endif
#ifdef RANDR
				if (have_randr && ev.xevent.type == randr_event_base + RRScreenChangeNotify) {
					handle_randr_event(&ev.xrandr);
				}
#endif
				break;
			}
		}
		if (need_client_tidy) {
			struct list *iter, *niter;
			need_client_tidy = 0;
			for (iter = clients_tab_order; iter; iter = niter) {
				Client *c = iter->data;
				niter = iter->next;
				if (c->remove)
					remove_client(c);
			}
		}
	}
}

/* interruptibleXNextEvent() is taken from the Blender source and comes with
 * the following copyright notice: */

/* Copyright (c) Mark J. Kilgard, 1994, 1995, 1996. */

/* This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain. */

/* Unlike XNextEvent, if a signal arrives, interruptibleXNextEvent will
 * return zero. */

static int interruptibleXNextEvent(XEvent *event) {
	fd_set fds;
	int rc;
	int dpy_fd = ConnectionNumber(dpy);
	for (;;) {
		if (XPending(dpy)) {
			XNextEvent(dpy, event);
			return 1;
		}
		FD_ZERO(&fds);
		FD_SET(dpy_fd, &fds);
		rc = select(dpy_fd + 1, &fds, NULL, NULL, NULL);
		if (rc < 0) {
			if (errno == EINTR) {
				return 0;
			} else {
				LOG_ERROR("interruptibleXNextEvent(): select()\n");
			}
		}
	}
}
