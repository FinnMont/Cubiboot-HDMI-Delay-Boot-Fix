#include <gctypes.h>
#include <ogc/video_types.h>

void ogc__VIInit(u32 vimode);

// Live black picture during preboot_delay_ms (see video.c). Call begin after the
// IPL's video setup has reached the hardware (a retrace or two in), and end once the
// wait is over; end is only needed when begin returned true.
bool preboot_signal_begin(u32 vimode, u16 dispSizeY);
void preboot_signal_end(void);

