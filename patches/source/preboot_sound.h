#include <gctypes.h>

// Faint sound during preboot_delay_ms (see preboot_sound.c). volume is 1-100, 0 = off.
// end is only needed when begin returned PREBOOT_SOUND_PLAYING.
enum {
	PREBOOT_SOUND_OFF = 0,          // volume 0 / not set
	PREBOOT_SOUND_PLAYING,          // playing
	PREBOOT_SOUND_DMA_BUSY,         // AI DMA already running: someone owns the audio
	PREBOOT_SOUND_IRQ_BUSY,         // AI DMA interrupt already unmasked: someone owns it
	PREBOOT_SOUND_NOT_ADVANCING,    // started the DMA but it never moved: stopped again
};

int preboot_sound_begin(u32 volume);
void preboot_sound_end(void);
