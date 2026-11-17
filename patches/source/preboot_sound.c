#include <gctypes.h>

#include "preboot_sound.h"

// ---------------------------------------------------------------------------------
// Faint sound during preboot_delay_ms (preboot_sound_volume)
//
// On a cold start some TVs keep their speakers muted until they hear real sound, then
// take a few seconds to unmute -- so the first seconds of the boot animation's sound are
// lost however long the black hold is. Playing a faint sound during the hold starts that
// timer early, so the TV has already unmuted when the animation's own sound begins.
//
// During the hold the IPL has not set up its sound system yet, so this drives the audio
// interface's DMA directly: a short buffer of soft noise in our own memory, looped by the
// hardware (the AI DMA replays the same block until it is reprogrammed). No DSP, no
// interrupts: the AI DMA interrupt stays masked and its status flag is cleared at the end.
//
// It only plays if nobody owns the audio DMA yet (DMA off, its interrupt masked), and
// only if the DMA is seen to actually advance; otherwise it does nothing at all.
// preboot_sound_end() puts every register it touched back as it found it, so the IPL's
// own audio setup starts from exactly the state it would have had.
//
// Register layout follows libogc's audio.c (AUDIO_InitDMA / AUDIO_StartDMA / __AIDHandler).
// ---------------------------------------------------------------------------------

static vu16* const _dspReg = (u16*)0xCC005000;

#define DSPREG_CSR          5   // DSP control/status (0xCC00500A)
#define DSPREG_AIDMA_HI     24  // AI DMA start address [28:16]
#define DSPREG_AIDMA_LO     25  // AI DMA start address [15:5]
#define DSPREG_AIDMA_CTRL   27  // bit 15 = enable, [14:0] = length in 32-byte blocks
#define DSPREG_AIDMA_LEFT   29  // blocks left in the current pass

#define DSPCR_DSPINT        0x0080  // write 1 to clear
#define DSPCR_ARINT         0x0020  // write 1 to clear
#define DSPCR_AIINTMSK      0x0010
#define DSPCR_AIINT         0x0008  // write 1 to clear

#define AIDMA_ENABLE        0x8000

// 8192 stereo 16-bit frames: ~170 ms at 48 kHz, ~256 ms at 32 kHz, then the hardware loops it.
#define SOUND_BYTES         (32 * 1024)
static s16 preboot_sound_buf[SOUND_BYTES / 2] __attribute__((aligned(32)));

static u16 saved_hi, saved_lo, saved_ctrl;
static bool sound_active = false;

static void sound_flush_dcache(void *ptr, u32 len) {
	u32 addr = (u32)ptr & ~31;
	u32 end = (u32)ptr + len;
	for (; addr < end; addr += 32) {
		asm volatile("dcbf 0,%0" : : "r"(addr) : "memory");
	}
	asm volatile("sync" : : : "memory");
}

static inline u32 read_tbl(void) {
	u32 t;
	asm volatile("mftb %0" : "=r"(t));
	return t;
}

static inline u32 irq_disable(void) {
	u32 msr;
	asm volatile("mfmsr %0" : "=r"(msr));
	asm volatile("mtmsr %0" : : "r"(msr & ~0x8000) : "memory");
	return msr;
}

static inline void irq_restore(u32 msr) {
	asm volatile("mtmsr %0" : : "r"(msr) : "memory");
}

static void fill_soft_noise(u32 volume) {
	// volume 1-100 -> peak amplitude 10-1000 out of 32767 (about -70 to -30 dBFS)
	const s32 amp = (s32)volume * 10;
	u32 rng = 0x2545F491;
	s32 smooth = 0;

	for (u32 i = 0; i < SOUND_BYTES / 4; i++) {
		rng ^= rng << 13;
		rng ^= rng >> 17;
		rng ^= rng << 5;
		s32 white = (s32)(rng % (u32)(2 * amp + 1)) - amp;
		smooth += (white - smooth) / 4;  // gentle low-pass: a soft hiss, not a harsh one
		preboot_sound_buf[i * 2] = (s16)smooth;      // left
		preboot_sound_buf[i * 2 + 1] = (s16)smooth;  // right
	}
	sound_flush_dcache(preboot_sound_buf, sizeof(preboot_sound_buf));
}

bool preboot_sound_begin(u32 volume) {
	sound_active = false;
	if (volume == 0) return false;
	if (volume > 100) volume = 100;

	// Leave the audio alone if anything already owns it.
	if (_dspReg[DSPREG_AIDMA_CTRL] & AIDMA_ENABLE) return false;
	if (_dspReg[DSPREG_CSR] & DSPCR_AIINTMSK) return false;

	fill_soft_noise(volume);

	saved_hi = _dspReg[DSPREG_AIDMA_HI];
	saved_lo = _dspReg[DSPREG_AIDMA_LO];
	saved_ctrl = _dspReg[DSPREG_AIDMA_CTRL];

	u32 pa = (u32)preboot_sound_buf & 0x3fffffff; // physical address
	u16 blocks = SOUND_BYTES / 32;

	_dspReg[DSPREG_AIDMA_HI] = (saved_hi & ~0x1fff) | ((pa >> 16) & 0x1fff);
	_dspReg[DSPREG_AIDMA_LO] = (saved_lo & ~0xffe0) | (pa & 0xffe0);
	_dspReg[DSPREG_AIDMA_CTRL] = (saved_ctrl & ~0x7fff) | (blocks & 0x7fff);
	_dspReg[DSPREG_AIDMA_CTRL] = _dspReg[DSPREG_AIDMA_CTRL] | AIDMA_ENABLE;
	sound_active = true;

	// Make sure the DMA is really running (the audio clock is up): give it up to ~10 ms
	// to move on, else stop and put everything back.
	const u32 timeout_ticks = 405000; // time base runs at 40.5 MHz on GameCube
	u16 left = _dspReg[DSPREG_AIDMA_LEFT];
	u32 start = read_tbl();
	while (_dspReg[DSPREG_AIDMA_LEFT] == left) {
		if (read_tbl() - start > timeout_ticks) {
			preboot_sound_end();
			return false;
		}
	}
	return true;
}

void preboot_sound_end(void) {
	if (!sound_active) return;

	// Stop, then restore the DMA address and length exactly as they were.
	_dspReg[DSPREG_AIDMA_CTRL] = _dspReg[DSPREG_AIDMA_CTRL] & ~AIDMA_ENABLE;
	_dspReg[DSPREG_AIDMA_HI] = saved_hi;
	_dspReg[DSPREG_AIDMA_LO] = saved_lo;
	_dspReg[DSPREG_AIDMA_CTRL] = saved_ctrl;

	// Clear the AI DMA interrupt flag our playback raised (it stayed masked), without
	// touching the DSP or ARAM interrupt flags, which are write-1-to-clear as well.
	u32 msr = irq_disable();
	u16 csr = _dspReg[DSPREG_CSR];
	if (csr & DSPCR_AIINT) {
		_dspReg[DSPREG_CSR] = (csr & ~(DSPCR_DSPINT | DSPCR_ARINT)) | DSPCR_AIINT;
	}
	irq_restore(msr);

	sound_active = false;
}
