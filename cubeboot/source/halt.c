#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <ogcsys.h>
#include <gccore.h>
#include <ogc/machine/processor.h>
#include <ogc/context.h>

#include "config.h"
#include "print.h"

extern GXRModeObj *rmode;

#ifndef CONSOLE_ENABLE
static void *exception_xfb = (void*)0xC1700000;
extern void VIDEO_SetFramebuffer(void *);
#endif

// The screen can be blanked when a message needs showing (libogc starts out blanked, and a
// crash can come before the loader switches its picture on), so anything that shows a
// message switches the picture on first. This does it by writing the VI registers directly, so it
// also works with interrupts off (exception handlers), where VIDEO_SetBlack()/VIDEO_Flush()
// would only take effect at the next retrace interrupt. It is the inverse of libogc's
// blanking (__setVerticalRegs): blanking moves a field's active lines into its pre- and
// post-blanking counts (+lines each) and sets the active line count in VTR to zero.
void loader_unblank_video(void) {
    if (rmode == NULL) return;

    vu16 *vi = (vu16*)0xCC002000;
    u16 vtr = vi[0];
    if ((vtr >> 4) != 0) return; // already showing a picture

    u32 nonInter = rmode->viTVMode & 0x03;
    u32 div1 = (nonInter == VI_PROGRESSIVE || nonInter == VI_3D) ? 1 : 2;
    s32 lines = rmode->viHeight / div1;
    if (lines <= 0 || lines > 0x3ff) return;

    s32 psb_odd = (s32)vi[6] - lines;
    s32 prb_odd = (s32)vi[7] - lines;
    s32 psb_even = (s32)vi[8] - lines;
    s32 prb_even = (s32)vi[9] - lines;
    if (psb_odd < 1 || prb_odd < 1 || psb_even < 1 || prb_even < 1) return;

    vi[6] = (u16)psb_odd;
    vi[7] = (u16)prb_odd;
    vi[8] = (u16)psb_even;
    vi[9] = (u16)prb_even;
    vi[0] = (u16)((lines << 4) | (vtr & 0x0f));
}

// libogc's crash screen ("Exception (DSI) occurred!") draws into the current framebuffer
// but never switches the picture on. The loader links with
// -Wl,--wrap=c_default_exceptionhandler (see the Makefile), so it comes through here first.
extern void __real_c_default_exceptionhandler(frame_context *ctx);
void __wrap_c_default_exceptionhandler(frame_context *ctx) {
    loader_unblank_video();
    __real_c_default_exceptionhandler(ctx);
}

void prog_halt(char *msg) {
    // make sure the picture is on so the message shows
    if (rmode != NULL) {
        VIDEO_SetBlack(FALSE);
        VIDEO_Flush();
        loader_unblank_video();
    }
#ifndef CONSOLE_ENABLE
	VIDEO_SetFramebuffer(exception_xfb);
    console_init(exception_xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    // CON_EnableGecko(EXI_CHANNEL_1, false);
#ifdef VIDEO_ENABLE
    VIDEO_WaitVSync();
#endif
#endif
    iprintf("\n====\nHALT\n====\n");
    iprintf(msg);
#ifdef VIDEO_ENABLE
    VIDEO_WaitVSync();
#endif
    ppchalt();
}
