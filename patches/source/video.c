#include <gctypes.h>
#include <ogc/gx_struct.h>
#include <ogc/video_types.h>

static vu16* const _viReg = (u16*)0xCC002000;

static const struct _timing {
	u8 equ;
	u16 acv;
	u16 prbOdd,prbEven;
	u16 psbOdd,psbEven;
	u8 bs1,bs2,bs3,bs4;
	u16 be1,be2,be3,be4;
	u16 nhlines,hlw;
	u8 hsy,hcs,hce,hbe640;
	u16 hbs640;
	u8 hbeCCIR656;
	u16 hbsCCIR656;
} video_timing[] = {
	{
		0x06,0x00F0,
		0x0018,0x0019,0x0003,0x0002,
		0x0C,0x0D,0x0C,0x0D,
		0x0208,0x0207,0x0208,0x0207,
		0x020D,0x01AD,
		0x40,0x47,0x69,0xA2,
		0x0175,
		0x7A,0x019C
	},
	{
		0x06,0x00F0,
		0x0018,0x0018,0x0004,0x0004,
		0x0C,0x0C,0x0C,0x0C,
		0x0208,0x0208,0x0208,0x0208,
		0x020E,0x01AD,
		0x40,0x47,0x69,0xA2,
		0x0175,
		0x7A,0x019C
	},
	{
		0x05,0x0120,
		0x0021,0x0022,0x0001,0x0000,
		0x0D,0x0C,0x0B,0x0A,
		0x026B,0x026A,0x0269,0x026C,
		0x0271,0x01B0,
		0x40,0x4B,0x6A,0xAC,
		0x017C,
		0x85,0x01A4
	},
	{
		0x05,0x0120,
		0x0021,0x0021,0x0000,0x0000,
		0x0D,0x0B,0x0D,0x0B,
		0x026B,0x026D,0x026B,0x026D,
		0x0270,0x01B0,
		0x40,0x4B,0x6A,0xAC,
		0x017C,
		0x85,0x01A4
	},
	{
		0x06,0x00F0,
		0x0018,0x0019,0x0003,0x0002,
		0x10,0x0F,0x0E,0x0D,
		0x0206,0x0205,0x0204,0x0207,
		0x020D,0x01AD,
		0x40,0x4E,0x70,0xA2,
		0x0175,
		0x7A,0x019C
	},
	{
		0x06,0x00F0,
		0x0018,0x0018,0x0004,0x0004,
		0x10,0x0E,0x10,0x0E,
		0x0206,0x0208,0x0206,0x0208,
		0x020E,0x01AD,
		0x40,0x4E,0x70,0xA2,
		0x0175,
		0x7A,0x019C
	},
	{
		0x0C,0x01E0,
		0x0030,0x0030,0x0006,0x0006,
		0x18,0x18,0x18,0x18,
		0x040E,0x040E,0x040E,0x040E,
		0x041A,0x01AD,
		0x40,0x47,0x69,0xA2,
		0x0175,
		0x7A,0x019C
	},
	{
		0x0C,0x01E0,
		0x002C,0x002C,0x000A,0x000A,
		0x18,0x18,0x18,0x18,
		0x040E,0x040E,0x040E,0x040E,
		0x041A,0x01AD,
		0x40,0x47,0x69,0xA8,
		0x017B,
		0x7A,0x019C
	},
	{
		0x0A,0x0240,
		0x0044,0x0044,0x0000,0x0000,
		0x14,0x14,0x14,0x14,
		0x04D8,0x04D8,0x04D8,0x04D8,
		0x04E2,0x01B0,
		0x40,0x4B,0x6A,0xAC,
		0x017C,
		0x85,0x01A4
	}
};

static const struct _timing* __gettiming(u32 vimode)
{
	switch(vimode) {
		case VI_TVMODE_NTSC_INT:
		case VI_TVMODE_EURGB60_INT:
			return &video_timing[0];
			break;
		case VI_TVMODE_NTSC_DS:
		case VI_TVMODE_EURGB60_DS:
			return &video_timing[1];
			break;
		case VI_TVMODE_PAL_INT:
		case VI_TVMODE_DEBUG_PAL_INT:
			return &video_timing[2];
			break;
		case VI_TVMODE_PAL_DS:
		case VI_TVMODE_DEBUG_PAL_DS:
			return &video_timing[3];
			break;
		case VI_TVMODE_MPAL_INT:
			return &video_timing[4];
			break;
		case VI_TVMODE_MPAL_DS:
			return &video_timing[5];
			break;
		case VI_TVMODE_NTSC_PROG:
		case VI_TVMODE_MPAL_PROG:
		case VI_TVMODE_EURGB60_PROG:
			return &video_timing[6];
			break;
		case VI_TVMODE_NTSC_3D:
			return &video_timing[7];
			break;
		case VI_TVMODE_PAL_PROG:
			return &video_timing[8];
			break;
		default:
			break;
	}
	return NULL;
}

void ogc__VIInit(u32 vimode) {
	vu32 cnt;
	u32 vi_mode,interlace,progressive;
	const struct _timing *cur_timing = NULL;

	vi_mode = ((vimode>>2)&0x07);
	interlace = (vimode&0x01);
	progressive = (vimode&0x02);

	cur_timing = __gettiming(vimode);

	//reset the interface
	_viReg[1] = 0x02;
	cnt = 0;
	while(cnt<1000) cnt++;
	_viReg[1] = 0x00;

	// now begin to setup the interface
	_viReg[2] = ((cur_timing->hcs<<8)|cur_timing->hce);		//set HCS & HCE
	_viReg[3] = cur_timing->hlw;							//set Half Line Width

	_viReg[4] = (cur_timing->hbs640<<1);					//set HBS640
	_viReg[5] = ((cur_timing->hbe640<<7)|cur_timing->hsy);	//set HBE640 & HSY

	_viReg[0] = cur_timing->equ;

	_viReg[6] = (cur_timing->psbOdd+2);							//set PSB odd field
	_viReg[7] = (cur_timing->prbOdd+((cur_timing->acv<<1)-2));	//set PRB odd field

	_viReg[8] = (cur_timing->psbEven+2);						//set PSB even field
	_viReg[9] = (cur_timing->prbEven+((cur_timing->acv<<1)-2));	//set PRB even field

	_viReg[10] = ((cur_timing->be3<<5)|cur_timing->bs3);		//set BE3 & BS3
	_viReg[11] = ((cur_timing->be1<<5)|cur_timing->bs1);		//set BE1 & BS1

	_viReg[12] = ((cur_timing->be4<<5)|cur_timing->bs4);		//set BE4 & BS4
	_viReg[13] = ((cur_timing->be2<<5)|cur_timing->bs2);		//set BE2 & BS2

	_viReg[24] = (0x1000|((cur_timing->nhlines/2)+1));
	_viReg[25] = (cur_timing->hlw+1);

	_viReg[26] = 0x1001;		//set DI1
	_viReg[27] = 0x0001;		//set DI1
	_viReg[36] = 0x2828;		//set HSR

	if(vi_mode<VI_PAL && vi_mode>=VI_DEBUG_PAL) vi_mode = VI_NTSC;
	if(progressive){
		_viReg[1] = ((vi_mode<<8)|0x0005);		//set MODE & INT & enable
		_viReg[54] = 0x0001;
	} else {
		_viReg[1] = ((vi_mode<<8)|(interlace<<2)|0x0001);
		_viReg[54] = 0x0000;
	}
}

// ---------------------------------------------------------------------------------
// Live black picture during preboot_delay_ms
//
// The IPL keeps the screen blanked (VISetBlack) until the first frame of the boot
// animation. Blanking programs zero active lines into VTR and moves them into the
// pre/post-blanking counts, so the digital AV port carries no picture at all and a
// GCVideo-based HDMI adapter has no mode to lock onto. The adapter and TV then only
// start syncing once the animation is already playing, and the start of it is lost --
// which is exactly what preboot_delay_ms is meant to prevent.
//
// So for the length of the delay we un-blank the VI ourselves, in the exact picture
// size and position the IPL will un-blank to when the animation starts. That part
// matters: GCVideo times its output from where the console's picture starts and how
// tall it is, so if our picture and the IPL's differ even by a line, the TV loses the
// signal and re-syncs just as the animation begins. The size and position come
// straight from the registers the IPL programmed, by undoing its blanking: Nintendo's
// SDK moves all but one of the active lines into pre-blanking and the last one into
// post-blanking; current libogc2 splits them evenly. Both are recognised.
//
// The black itself comes one of two ways:
//  - Handover (normal case): the IPL's own framebuffer is cleared to black and the VI
//    keeps scanning it. preboot_signal_end() then touches nothing at all -- the
//    picture stays on, already in the IPL's size and position, and when the IPL
//    un-blanks for the animation it writes the very values that are already there.
//    The signal never changes from the start of the hold into the animation.
//  - Fallback, if the framebuffer the VI points at isn't plainly the IPL's (outside
//    MEM1, or in memory Cubiboot uses): scan one line of black from our own memory
//    (stride 0, so every line reads the same 2 KiB), and at the end put every register
//    back exactly as the IPL left it.
//
// Timing registers are only ever written in the first few lines of a field, before
// its picture starts, so no field is half old values and half new.
//
// This all runs inside pre_menu_init, while the IPL's own main loop is waiting on us,
// so nothing else programs the VI in between. Register layout and the blanking
// arithmetic follow libogc's video.c (__setVerticalRegs, __setFbbRegs, __setPicConfig).
// ---------------------------------------------------------------------------------

#define VI_YUV_BLACK 0x10801080 // Y=16 Cb=128 Y=16 Cr=128

#define VIREG_VTR      0  // (active lines << 4) | equ
#define VIREG_DCR      1
#define VIREG_PSB_ODD  6
#define VIREG_PRB_ODD  7
#define VIREG_PSB_EVEN 8
#define VIREG_PRB_EVEN 9
#define VIREG_TFBL_HI  14 // (POFF << 12) | (XOF << 8) | address[23:16]
#define VIREG_TFBL_LO  15
#define VIREG_BFBL_HI  18
#define VIREG_BFBL_LO  19
#define VIREG_VCT      22 // line of the frame the VI is on now, counting from 1
#define VIREG_PICCONF  36 // (words per line << 8) | stride

static const u8 preboot_regs[] = {
	VIREG_VTR, VIREG_PSB_ODD, VIREG_PRB_ODD, VIREG_PSB_EVEN, VIREG_PRB_EVEN,
	VIREG_TFBL_HI, VIREG_TFBL_LO, VIREG_BFBL_HI, VIREG_BFBL_LO, VIREG_PICCONF,
};
#define PREBOOT_NREGS (sizeof(preboot_regs) / sizeof(preboot_regs[0]))

static u32 preboot_black_line[512] __attribute__((aligned(32))); // 2 KiB: up to 64 words per line
static u16 preboot_saved[PREBOOT_NREGS];
static bool preboot_active = false;
static bool preboot_handover = false;
static const struct _timing *preboot_timing = NULL;

static void preboot_flush_dcache(void *ptr, u32 len) {
	u32 addr = (u32)ptr & ~31;
	u32 end = (u32)ptr + len;
	for (; addr < end; addr += 32) {
		asm volatile("dcbf 0,%0" : : "r"(addr) : "memory");
	}
	asm volatile("sync" : : : "memory");
}

static inline u32 preboot_irq_disable(void) {
	u32 msr;
	asm volatile("mfmsr %0" : "=r"(msr));
	asm volatile("mtmsr %0" : : "r"(msr & ~0x8000) : "memory"); // clear EE
	return msr;
}

static inline void preboot_irq_restore(u32 msr) {
	asm volatile("mtmsr %0" : : "r"(msr) : "memory");
}

// Is the VI in the first few lines of a field -- vertical sync and equalisation, well
// before the picture? The line counter runs over the whole frame from 1; an interlaced
// frame's second field starts halfway through (nhlines counts half-lines per field), on
// a line it shares with the end of the first, which is skipped. A progressive frame is
// one field, so its "halfway" line is past the end and never matches.
static bool preboot_in_field_start(void) {
	const u16 window = 6; // lines; the picture never starts before ~20 lines in
	const u16 second = (preboot_timing->nhlines / 2) + 1;
	u16 line = _viReg[VIREG_VCT] & 0x7ff;
	return (line >= 1 && line < 1 + window) || (line > second && line < second + window);
}

// Busy-waits for the start of a field, so registers written next apply to the whole of
// it, and returns with interrupts off so nothing can delay those writes past it (hand
// *msr to preboot_irq_restore afterwards). Returns false, interrupts untouched, after
// roughly two fields without catching one.
static bool preboot_wait_field_start(u32 *msr) {
	for (u32 n = 0; n < 400000; n++) {
		if (!preboot_in_field_start()) continue;
		*msr = preboot_irq_disable();
		if (preboot_in_field_start()) return true;
		preboot_irq_restore(*msr);
	}
	return false;
}

static u32 preboot_fb_phys(u16 hi, u16 lo, bool poff) {
	u32 addr = ((u32)(hi & 0xff) << 16) | lo;
	return poff ? (addr << 5) : addr;
}

// The memory the VI scans for both fields with the IPL's framebuffer settings, if it is
// plainly RAM the IPL owns: inside MEM1, above the OS globals, and clear of Cubiboot's
// own memory (its low-memory block, and the patches plus the loader's framebuffer from
// 0x81600000 up).
static bool preboot_ipl_fb_region(u32 lines, u32 *start, u32 *len) {
	bool poff = (preboot_saved[5] >> 12) & 1;
	u32 top = preboot_fb_phys(preboot_saved[5], preboot_saved[6], poff);
	u32 bottom = preboot_fb_phys(preboot_saved[7], preboot_saved[8], poff);
	u32 wpl = (preboot_saved[9] >> 8) & 0x7f;
	u32 stride = preboot_saved[9] & 0xff;
	if (wpl == 0 || stride < wpl) return false;

	// one field: (lines - 1) strides plus the last line, plus a word for the XOF shift
	u32 field_bytes = stride * 32 * (lines - 1) + wpl * 32 + 32;
	u32 lo = (top < bottom ? top : bottom) & ~31;
	u32 hi = ((top > bottom ? top : bottom) + field_bytes + 31) & ~31;
	if (hi <= lo || hi - lo > 0x200000) return false; // no framebuffer is over 2 MiB

	if (lo < 0x00004000 || hi > 0x01600000) return false;
	if (lo < 0x00500000 && hi > 0x00100000) return false;

	*start = lo;
	*len = hi - lo;
	return true;
}

// Returns true when it changed the VI, in which case preboot_signal_end() must be
// called before the IPL draws anything. On anything unexpected it changes nothing
// and returns false, which leaves the delay working exactly as it did before.
bool preboot_signal_begin(u32 vimode, u16 dispSizeY) {
	preboot_active = false;
	preboot_handover = false;

	preboot_timing = __gettiming(vimode);
	const struct _timing *tm = preboot_timing;
	if (tm == NULL) return false;

	// The VI has to be running and out of reset (the delay loop relies on it anyway).
	u16 dcr = _viReg[VIREG_DCR];
	if (!(dcr & 0x0001) || (dcr & 0x0002)) return false;

	for (u32 i = 0; i < PREBOOT_NREGS; i++) {
		preboot_saved[i] = _viReg[preboot_regs[i]];
	}

	// Only act on exactly what we expect to find: the IPL's mode, blanked.
	u16 vtr = preboot_saved[0];
	if ((vtr & 0x0f) != tm->equ) return false;  // VI not in the IPL's mode (yet)
	if ((vtr >> 4) != 0) return false;           // already showing a picture

	u32 nonInter = vimode & 0x03;
	bool progressive = (nonInter == VI_PROGRESSIVE || nonInter == VI_3D);
	s32 lines = progressive ? dispSizeY : dispSizeY / 2; // active lines per field
	if (lines < 2 || lines > tm->acv) return false;

	// Undo the IPL's blanking to get the pre/post-blanking of its picture. Values are
	// checked against the mode's own minimums, which the wrong split fails.
	s32 min_prb = tm->prbOdd < tm->prbEven ? tm->prbOdd : tm->prbEven;
	s32 min_psb = tm->psbOdd < tm->psbEven ? tm->psbOdd : tm->psbEven;
	s32 psb_odd, prb_odd, psb_even, prb_even;
	bool found = false;
	for (u32 split = 0; split < 2 && !found; split++) {
		s32 to_prb = split == 0 ? 2 * lines - 2 : lines; // SDK, then libogc2
		s32 to_psb = split == 0 ? 2 : lines;
		psb_odd = (s32)preboot_saved[1] - to_psb;
		prb_odd = (s32)preboot_saved[2] - to_prb;
		psb_even = (s32)preboot_saved[3] - to_psb;
		prb_even = (s32)preboot_saved[4] - to_prb;
		found = prb_odd >= min_prb && prb_even >= min_prb && prb_odd <= 0x3ff && prb_even <= 0x3ff &&
			psb_odd >= min_psb && psb_even >= min_psb && psb_odd <= 0x3ff && psb_even <= 0x3ff;
	}
	if (!found) return false;

	u32 fb_start = 0, fb_len = 0;
	u16 picconf = preboot_saved[9];
	u32 fb = 0;
	u16 poff = 0;
	preboot_handover = preboot_ipl_fb_region(lines, &fb_start, &fb_len);
	if (preboot_handover) {
		// The IPL hasn't drawn anything yet; it renders its first frame over this.
		u32 *p = (u32*)(fb_start | 0x80000000);
		for (u32 i = 0; i < fb_len / 4; i++) {
			p[i] = VI_YUV_BLACK;
		}
		preboot_flush_dcache(p, fb_len);
	} else {
		// Same words per line as the IPL, stride 0: every line scans the same black line.
		u32 wpl = (picconf >> 8) & 0x7f;
		if (wpl == 0 || wpl * 32 > sizeof(preboot_black_line)) return false;

		for (u32 i = 0; i < sizeof(preboot_black_line) / sizeof(u32); i++) {
			preboot_black_line[i] = VI_YUV_BLACK;
		}
		preboot_flush_dcache(preboot_black_line, sizeof(preboot_black_line));

		fb = (u32)preboot_black_line & 0x3fffffff; // physical address
		if (fb >= 0x01000000) {
			fb >>= 5;
			poff = 1;
		}
	}

	u32 msr;
	if (!preboot_wait_field_start(&msr)) {
		preboot_handover = false;
		return false;
	}

	if (!preboot_handover) {
		// Point the scanout at the black line first, then give the fields their
		// active lines, so no field ever shows anything else.
		_viReg[VIREG_TFBL_HI] = (poff << 12) | ((fb >> 16) & 0xff);
		_viReg[VIREG_TFBL_LO] = fb & 0xffff;
		_viReg[VIREG_BFBL_HI] = (fb >> 16) & 0xff;
		_viReg[VIREG_BFBL_LO] = fb & 0xffff;
		_viReg[VIREG_PICCONF] = picconf & 0xff00;
	}

	_viReg[VIREG_PSB_ODD] = (u16)psb_odd;
	_viReg[VIREG_PRB_ODD] = (u16)prb_odd;
	_viReg[VIREG_PSB_EVEN] = (u16)psb_even;
	_viReg[VIREG_PRB_EVEN] = (u16)prb_even;
	_viReg[VIREG_VTR] = (u16)((lines << 4) | tm->equ);
	preboot_irq_restore(msr);

	preboot_active = true;
	return true;
}

void preboot_signal_end(void) {
	if (!preboot_active) return;
	preboot_active = false;

	// Handover: leave the picture on. It already has the IPL's size, position and
	// framebuffer, so the IPL un-blanking for the animation changes nothing on the wire.
	if (preboot_handover) return;

	// Fallback: blank first, then put the IPL's framebuffer and stride back. If the
	// field start can't be caught, write anyway -- the IPL still has to get its
	// registers back.
	u32 msr;
	bool irq_off = preboot_wait_field_start(&msr);
	_viReg[VIREG_VTR] = preboot_saved[0];
	_viReg[VIREG_PSB_ODD] = preboot_saved[1];
	_viReg[VIREG_PRB_ODD] = preboot_saved[2];
	_viReg[VIREG_PSB_EVEN] = preboot_saved[3];
	_viReg[VIREG_PRB_EVEN] = preboot_saved[4];

	_viReg[VIREG_TFBL_HI] = preboot_saved[5];
	_viReg[VIREG_TFBL_LO] = preboot_saved[6];
	_viReg[VIREG_BFBL_HI] = preboot_saved[7];
	_viReg[VIREG_BFBL_LO] = preboot_saved[8];
	_viReg[VIREG_PICCONF] = preboot_saved[9];
	if (irq_off) preboot_irq_restore(msr);
}
