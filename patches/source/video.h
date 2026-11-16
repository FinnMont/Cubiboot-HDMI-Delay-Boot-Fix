#include <gctypes.h>
#include <ogc/video_types.h>

void ogc__VIInit(u32 vimode);

// Live black picture during preboot_delay_ms (see video.c). Call begin and end right
// after a VIWaitForRetrace(); end is only needed when begin returned true.
bool preboot_signal_begin(u32 vimode, u16 dispSizeY, u16 dispPosY);
void preboot_signal_end(void);

