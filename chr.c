#include "lib8001.h"
#include "chr.h"
#include "pattern.h"

#define SPRITE_CN		36

typedef struct {
	Sprite s;
	s8 timer;
	u8 *ptr;
} SpriteC;

static SpriteC spriteC[SPRITE_CN];
static SpriteContext ctx_c;
static s8 chrX, chrY, time;
static u8 chrcolor;

void chrTime(u8 t) {
	time = t ? t : -1;
}

void chrColor(u8 color) {
	chrcolor = color;
}

void chrValue(u8 *buf, u8 len, u16 v) {
	static const u16 b[] = { 1, 10, 100, 1000, 10000 };
	u8 f = 0;
	if (len > 6) len = 6;
	for (s8 i = len - 2; i >= 0; i--) {
		u8 d = v / b[i];
		v -= d * b[i];
		f |= d | !i;
		*buf++ = f ? d + '0' : ' ';
	}
	*buf = 0;
}

void chrLocate(s8 x, s8 y) {
	chrX = 150 - (y << 2);
	chrY = 2 + 3 * x;
}

static u8 behaviorChr(Sprite *p) {
	SpriteC *s = (SpriteC *)p;
	if (!s->ptr) return s->timer < 0 || s->timer--;
	p->flags = s->timer ? 0 : SF_HIDDEN;
	u8 c = *s->ptr;
	if (c) {
		s16 f = c - ' ' << AS;
		if (p->frame != f) {
			spriteFrame(p, f);
			s->timer = 2;
		}
		else if (s->timer) s->timer--;
	}
	return 1;
}

Sprite *chrPut(u8 c) {
	spriteContext(&ctx_c);
	SpriteC *s = (SpriteC *)spriteCreate(0, pat_chr);
	if (s) {
		s->s.behavior = behaviorChr;
		s->s.x = chrX << PS;
		s->s.y = chrY << PS;
		if (chrcolor) spriteColor(&s->s, chrcolor);
		s->s.animSpeed = 0;
		spriteFrame(&s->s, c - ' ' << AS);
		s->timer = time;
		s->ptr = nil;
		chrY += 6;
	}
	spriteContext(nil);
	return &s->s;
}

void chrPuts(u8 *str) {
	u8 c;
	while (c = *str++) chrPut(c);
}

void chrPutsBuf(u8 *str) {
	u8 c;
	while (c = *str) {
		SpriteC *s = (SpriteC *)chrPut(c);
		s->timer = 2;
		s->ptr = str++;
	}
}

static void chrInitSub(void) {
	spriteContext(&ctx_c);
	spriteSetupArray(spriteC);
	spriteView(0, 0, 79, 25);
	spriteContext(nil);
}

void chrInit(void) {
	chrcolor = 0;
	chrX = 100;
	chrY = 0;
	time = 0;
	chrInitSub();
}

void chrUpdate(void) {
	spriteContext(&ctx_c);
	spriteUpdate();
	spriteContext(nil);
}

void chrFlush(void) {
	chrUpdate();
	chrInitSub();
}
