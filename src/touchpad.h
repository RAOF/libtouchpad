/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdbool.h>

#define ARRAY_LENGTH(_arr) (sizeof(_arr)/sizeof(_arr[0]))


struct touchpad;

#define TOUCHPAD_HANDLE_ALL_EVENTS -1

/**
 * Input parameter into the struct touchpad_interface::scroll
 * callback.
 */
enum touchpad_scroll_direction {
	TOUCHPAD_SCROLL_HORIZONTAL = 3,
	TOUCHPAD_SCROLL_VERTICAL = 4,
};

/**
 * Callback interface used by the backends to get notified about
 * events on the touchpad.
 */
struct touchpad_interface {
	/**
	 * Called for relative motion event compared to the previous position.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param x Unaccelerated but de-jittered x coordinate delta
	 * @param y Unaccelerated but de-jittered y coordinate delta
	 */
	void (*motion)(struct touchpad *tp, void *userdata, int x, int y);
	/**
	 * Called for a physical button event
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param button The button number pressed, see linux/input.h for a
	 *	  range of possible buttons.
	 * @param is_press True for a press event, false for a release event
	 */
	void (*button)(struct touchpad *tp,
		      void *userdata,
		      unsigned int button,
		      bool is_press);
	/**
	 * Called for a scroll event.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param direction The scrolling direction
	 * @param units The number of units scrolled in that direction
	 */
	void (*scroll)(struct touchpad *tp, void *userdata,
		       enum touchpad_scroll_direction direction, int units);
};

struct touchpad* touchpad_new_from_path(const char *path);
void touchpad_free(struct touchpad *tp);
int touchpad_set_fd(struct touchpad *tp, int fd);
int touchpad_reopen(struct touchpad *tp);
int touchpad_close(struct touchpad *tp);
int touchpad_get_min_max(struct touchpad *tp, int axis, int *min, int *max, int *res);
void touchpad_set_interface(struct touchpad *tp, const struct touchpad_interface *interface);
int touchpad_handle_events(struct touchpad *tp, void *userdata);
int touchpad_get_fd(struct touchpad *tp);
