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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xorg-server.h>
#include <exevents.h>
#include <xf86Xinput.h>
#include <xserver-properties.h>
#include <linux/input.h>
#include "touchpad.h"

#define TOUCHPAD_MAX_BUTTONS 7 /* three buttons, 4 scroll buttons */
#define TOUCHPAD_NUM_AXES 4 /* x, y, hscroll, vscroll */

static inline struct touchpad*
xf86touchpad(InputInfoPtr pInfo)
{
	return (struct touchpad*)pInfo->private;
}

static int
xf86touchpad_on(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct touchpad *tp = xf86touchpad(pInfo);
	int fd;

	fd = touchpad_reopen(tp);
	if (fd > -1) {
		pInfo->fd = fd;
		xf86AddEnabledDevice(pInfo);
		dev->public.on = TRUE;
	}

	return dev->public.on ? Success : !Success;
}

static int
xf86touchpad_off(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct touchpad *tp = xf86touchpad(pInfo);

	touchpad_close(tp);
	xf86RemoveEnabledDevice(pInfo);
	pInfo->fd = -1;
	dev->public.on = FALSE;
	return Success;

}

static void
xf86touchpad_ptr_ctl(DeviceIntPtr dev, PtrCtrl *ctl)
{
}


static void
init_button_map(unsigned char *btnmap, size_t size)
{
	int i;

	memset(btnmap, 0, size);
	for (i = 0; i <= TOUCHPAD_MAX_BUTTONS; i++)
		btnmap[i] = i;
}

static void
init_button_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
	labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
	labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
	labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
	labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
	labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
	labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
}

static void
init_axis_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
	labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
	labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HSCROLL);
	labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_VSCROLL);
}

static int
xf86touchpad_init(DeviceIntPtr dev)
{
	unsigned char btnmap[TOUCHPAD_MAX_BUTTONS + 1];
	Atom btnlabels[TOUCHPAD_MAX_BUTTONS];
	Atom axislabels[TOUCHPAD_NUM_AXES];

	dev->public.on = FALSE;

	init_button_map(btnmap, ARRAY_LENGTH(btnmap));
	init_button_labels(btnlabels, ARRAY_LENGTH(btnlabels));
	init_axis_labels(axislabels, ARRAY_LENGTH(axislabels));

	InitPointerDeviceStruct((DevicePtr)dev, btnmap,
				TOUCHPAD_MAX_BUTTONS,
				btnlabels,
				xf86touchpad_ptr_ctl,
				GetMotionHistorySize(),
				TOUCHPAD_NUM_AXES,
				axislabels);

	return Success;
}

static int
xf86touchpad_device_control(DeviceIntPtr dev, int mode)
{
	int rc = BadValue;

	switch(mode) {
		case DEVICE_INIT:
			rc = xf86touchpad_init(dev);
			break;
		case DEVICE_ON:
			rc = xf86touchpad_on(dev);
			break;
		case DEVICE_OFF:
		case DEVICE_CLOSE:
			rc = xf86touchpad_off(dev);
			break;
	}

	return rc;
}

void
xf86touchpad_motion(struct touchpad *tp, void *userdata, int x, int y)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;

	xf86PostMotionEvent(dev, Relative, 0, 2, x, y);
}

void
xf86touchpad_button(struct touchpad *tp, void *userdata, unsigned int button /* linux/input.h */, bool is_press)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;

	switch(button) {
		case BTN_LEFT: button = 1; break;
		case BTN_MIDDLE: button = 2; break;
		case BTN_RIGHT: button = 3; break;
		default: /* no touchpad actually has those buttons */
			return;
	}

	xf86PostButtonEvent(dev, Relative, button, is_press, 0, 0);
}

void
xf86touchpad_scroll(struct touchpad *tp, void *userdata,
		    enum touchpad_scroll_direction direction, int units)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;
	int first = (direction == TOUCHPAD_SCROLL_HORIZONTAL) ? 2 : 3;

	xf86PostMotionEvent(dev, Absolute, first, 1, units);
}

static const struct touchpad_interface xf86touchpad_interface = {
	.motion = xf86touchpad_motion,
	.button = xf86touchpad_button,
	.scroll = xf86touchpad_scroll,
};

static void
xf86touchpad_read_input(InputInfoPtr pInfo)
{
	struct touchpad *tp = xf86touchpad(pInfo);
	touchpad_handle_events(tp, pInfo);
}

static int xf86touchpad_pre_init(InputDriverPtr drv,
				 InputInfoPtr pInfo,
				 int flags)
{
	struct touchpad *tp;
	char *device;

	pInfo->fd = -1;
	pInfo->type_name = XI_TOUCHPAD;
	pInfo->device_control = xf86touchpad_device_control;
	pInfo->read_input = xf86touchpad_read_input;
	pInfo->control_proc = NULL;
	pInfo->switch_mode = NULL;

	device = xf86SetStrOption(pInfo->options, "Device", NULL);
	if (!device)
		goto fail;
	tp = touchpad_new_from_path(device);
	if (!tp)
		goto fail;

	touchpad_set_interface(tp, &xf86touchpad_interface);
	touchpad_close(tp);

	pInfo->private = tp;

	return Success;

fail:
	free(device);
	return BadValue;
}

static void
xf86touchpad_uninit(InputDriverPtr drv,
		    InputInfoPtr pInfo,
		    int flags)
{
	touchpad_free(pInfo->private);
}


InputDriverRec xf86touchpad_driver = {
	.driverVersion	= 1,
	.driverName	= "touchpad",
	.PreInit	= xf86touchpad_pre_init,
	.UnInit		= xf86touchpad_uninit,
};

static XF86ModuleVersionInfo xf86touchpad_version_info = {
	"touchpad",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}
};

static pointer
xf86touchpad_setup_proc(pointer module, pointer options, int *errmaj, int *errmin)
{
	xf86AddInputDriver(&xf86touchpad_driver, module, 0);
	return module;
}

_X_EXPORT XF86ModuleData touchpadModuleData = {
	.vers		= &xf86touchpad_version_info,
	.setup		= &xf86touchpad_setup_proc,
	.teardown	= NULL
};

