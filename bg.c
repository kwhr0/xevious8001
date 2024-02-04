#include "lib8001.h"
#include "bg.h"
#include "bgconf.h"

#define STAGE		1
#define BGTEST		0
#define XOFS		3
#define XLIM		69
#define LOOP		6

typedef struct {
	u8 *p;
	u8 c, d;
} ExpandWork;

static ExpandWork w[2][25];
static u8 *yofsp;
static u8 sw, stage;
static s16 xofs, xn;
static const u8 yofs[] = {
	58, 90, 13, 77, 32, 66, 0, 
	87, 52, 7, 77, 87, 40, 13, 67, 90
};

extern const u8 bgdata[];

void bgDraw(ExpandWork *w, ExpandWork *w2, u8 *dp, u8 n, u8 half) CC0;

void bgInit(void) {
	stage = STAGE - 1;
	xofs = 0;
}

s16 bgOfs(void) {
	return stage * (BG_XN + PRE_BLANK2) + xofs;
}

u8 bgStage(void) {
	return stage + 1;
}

static void setup(u8 index, u8 skip) {
	u8 ofs = *yofsp;
	ExpandWork *p = w[index];
	u16 *op = &((u16 *)bgdata)[ofs];
	for (u8 y = 0; y < 25; y++) {
		p->p = (u8 *)&bgdata[*op++];
		if (skip) p->p += 2;
		p->c = *p->p++;
		p->d = *p->p++;
		p++;
	}
	if (++yofsp >= yofs + sizeof(yofs)) yofsp = (u8 *)yofs + LOOP;
}

u8 bgUpdate(void) {
	u16 xl = xn - xofs + 1 >> 1;
	if (xl > XLIM) xl = XLIM;
	bgDraw(w[sw], w[!sw], vram + XOFS, xl, xofs & 1);
	if (++xofs >= xn) {
		xofs = 0;
		xn = BG_XN + PRE_BLANK2;
		setup(sw, 1);
		sw = !sw;
		if (++stage >= sizeof(yofs)) {
			stage = LOOP;
			return 1;
		}
	}
	return 0;
}

void bgStart(void) {
	if (xofs >= 7 * (BG_XN + PRE_BLANK2) / 10 && ++stage >= sizeof(yofs)) stage = LOOP;
	xofs = -PRE_BLANK1;
	xn = BG_XN + PRE_BLANK2;
	sw = 0;
	yofsp = (u8 *)yofs + stage;
	setup(0, 0);
	setup(1, 1);
#if BGTEST
	xofs += BGTEST << 1;
	for (u16 n = BGTEST; n-- > 0;) {
		ExpandWork *p = w[sw];
		for (u8 y = 0; y < 25; y++) {
			if (!--p->c) p->c = *p->p++, p->d = *p->p++;
			p++;
		}
	}
#endif
}
