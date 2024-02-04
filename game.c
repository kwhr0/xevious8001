#include "lib8001.h"
#include "game.h"
#include "sp.h"
#include "emitter.h"
#include "bg.h"
#include "chr.h"
#include "pattern.h"
#include "music.h"

#define RESERVE		2

#if RESERVE > 0
#define SPRITE_RN	RESERVE
#else
#define SPRITE_RN	1
#endif

static Sprite spriteR[SPRITE_RN];
static u8 demo, playf, trigger;
static u16 score, scoreNext, hiscore = 4000;
static u8 scoreBuf[6], hiscoreBuf[6];
static SpriteContext ctx_r, ctx_logo;
static s8 reserve;

void scoreReset(void) {
	score = 0;
	scoreNext = 2000;
}

u16 scoreGet(void) {
	return score;
}

void demoSet(u8 f) {
	demo = f;
}

u8 isDemo(void) {
	return demo;
}

void scorePrint(void) {
	chrTime(2);
	chrLocate(8, 0);
	chrPuts("1UP");
	chrLocate(18, 0);
	chrPuts("HISCORE");
	chrLocate(4, 2);
	chrValue(scoreBuf, sizeof(scoreBuf), score);
	chrPutsBuf(scoreBuf);
	chrPut('0');
	chrLocate(20, 2);
	chrValue(hiscoreBuf, sizeof(hiscoreBuf), hiscore);
	chrPutsBuf(hiscoreBuf);
	chrPut('0');
}

void scoreUpdate(void) {
	static u8 timer;
	if (!demo && !(++timer & 7)) {
		chrTime(2);
		chrLocate(8, 0);
		chrPuts(timer & 8 ? "1UP" : "   ");
	}
}

static void reserveSet(Pattern *pat) {
	spriteContext(&ctx_r);
	Sprite *p = spriteCreate(0, pat);
	if (p) {
		p->y = 6 * reserve << PS;
		p->animSpeed = 1 << AS - 1;
		p->flags = SF_ERASE_NO_ANIM;
	}
	spriteContext(nil);
}

void reserveInc(void) {
	reserveSet(pat_solvalou_r);
	reserve++;
}

u8 reserveDec(void) {
	reserve--;
	reserveSet(pat_solvalou_r_off);
	return reserve < 0;
}

void scoreAdd(u16 v) {
	if (demo) return;
	score += v;
	chrValue(scoreBuf, sizeof(scoreBuf), score);
	if (hiscore < score) hiscore = score;
	chrValue(hiscoreBuf, sizeof(hiscoreBuf), hiscore);
	if (score >= scoreNext) {
		if (scoreNext == 2000) scoreNext = 6000;
		else scoreNext += 6000;
		playStart(6, MUSIC_1UP, 0);
		reserveInc();
	}
}

u8 triggerGet(void) {
	u8 r = trigger & KEY5;
	trigger = 0xff;
	return r;
}

static void intKey(void) {
	trigger &= KEY5;
	idle();
}

void logoInit(void) {
	static Sprite sprite;
	spriteContext(&ctx_logo);
	spriteSetup(&sprite, 1, 0);
	Sprite *p = spriteCreate(0, pat_logo);
	if (p) {
		p->x = 90 << PS;
		p->y = 14 << PS;
	}
	spriteContext(nil);
}

void logoUpdate(void) {
	spriteContext(&ctx_logo);
	spriteUpdate();
	spriteContext(nil);
}

static void gameStart(void) {
	if (isDemo()) logoInit();
	else {
		chrTime(60);
		chrLocate(10, 18);
		chrPuts("READY!");
		u8 buf[3];
		chrValue(buf, sizeof(buf), reserve);
		chrLocate(reserve < 10 ? -1 : 0, 22);
		chrPuts(buf);
		chrPuts(" SOLVALOU LEFT");
	}
	playStart(6, MUSIC_START, 0);
	playf = 0;
}

static u8 gameUpdate(void) {
	spriteContext(&ctx_r);
	spriteUpdate();
	spriteContext(nil);
	u8 f = playing(6) == MUSIC_START;
	if (playf && !f) playStart(0, MUSIC_FLY, PF_LOOP);
	playf = f;
	if (isDemo()) {
		logoUpdate();
		if (!(KEY5 & 1)) {
			demoSet(0);
			return 0;
		}
	}
	return 1;
}

void gameInit(void) {
	reserve = 0;
	spriteContext(&ctx_r);
	spriteSetupArray(spriteR);
	spriteView(0, 0, 3, 25);
	spriteContext(nil);
#if RESERVE
	if (!isDemo()) for (u8 i = 0; i < RESERVE; i++) reserveInc();
#endif
	intSet(intKey);
	trigger = 0xff;
	cls();
	vramSwap(0);
	cls();
	chrInit();
	scorePrint();
	chrUpdate(); // write 1st
	vramSwap(0);
	chrUpdate(); // write 2nd
	chrUpdate(); // retire
	bgInit();
	emitterInit();
}

static void meter(u8 v) {
	u8 *p = vram;
	for (u8 i = 0; i < 25; i++, p += 120) 
		*p = i < v ? i & 1 ? 0xf0 : 0xf : 0;
}

void gameMain(void) {
	spInit();
	gameStart();
	bgStart();
	emitterStart(); // must be after bgStart()
	u8 r;
	do {
		if (bgUpdate()) emitterStart();
		emitterUpdate();
		r = spUpdate() && gameUpdate();
		spriteUpdate();
		scoreUpdate();
		chrUpdate();
#if 0
		meter(vramSwap(keyPress() ? 10 : 1));
#else
		vramSwap(3);
#endif
	} while (r);
	playStopAll();
}

void gameoverMain(void) {
	vramSingle();
	cls();
	chrInit();
	chrTime(0);
	chrLocate(7, 16);
	chrPuts("GAME OVER");
	scorePrint();
	for (u8 i = 0; i < 60; i++) {
		scoreUpdate();
		chrUpdate();
		waitVSync(3);
	}
}
