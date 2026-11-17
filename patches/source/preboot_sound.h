#include <gctypes.h>

// Faint sound during preboot_delay_ms (see preboot_sound.c). volume is 1-100, 0 = off.
// end is only needed when begin returned true.
bool preboot_sound_begin(u32 volume);
void preboot_sound_end(void);
