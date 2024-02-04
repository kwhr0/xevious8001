#include "lib8001.h"
#include "emitter.h"
#include "bg.h"

#define EMITTER_N		3

typedef struct {
	u16 timer;
	u8 nexttimer, type, groupN;
} Emitter;

extern GenTable gentable;
extern EmitterSch es[];

static const u8 interval[] = {
	30, // toroid
	10, // torkan
	10, // zoshi
	5, // giddospario
	20, // jara
	10, // kapi
	10, // terrazi
	10, // zakato_d
	20, // bragzakato
	10, // zakato_s
	255, // garuzakato
	10, // bacura
	10, // zoshi_b
};

static Emitter emitter[EMITTER_N];
static EmitterSch *esp;
static u8 diff, diffFrac, groupN, groupU;
static GroupFunc groupFunc;

void emitterInit(void) {
	diff = diffFrac = 0;
}

void emitterStart(void) {
	for (esp = es; esp->ofs != 0x7fff && bgOfs() >= esp->ofs; esp++)
		;
	for (Emitter *p = emitter; p < emitter + EMITTER_N; p++) p->timer = 0;
	groupN = 0;
}

void diffAdd(s8 v) {
	s16 t = diff + v;
	if (t < 0) t = 0;
	else if (t >= 128) t = 64;
	diff = t;
}

void diffAddFrac(u8 v) {
	u8 l = diffFrac;
	diffFrac += v;
	if (diffFrac < l) diffAdd(1);
}

u8 diffGet(void) {
	return diff;
}

void emitterGroup(GroupFunc func, u8 u) {
	groupN = 2 + (diff >> 5) + R(3);
	groupU = u;
	groupFunc = func;
}

s16 emitterPrm(void) {
	return esp->prm;
}

void emitterUpdate(void) {
	Emitter *p;
	if (groupN) {
		groupN--;
		groupFunc(R(75) + 10, groupU);
	}
	diffAddFrac(2);
	for (; esp->ofs != 0x7fff && bgOfs() >= esp->ofs; esp++)
		if (esp->type == DIFF) diffAdd(esp->prm);
		else if (esp->type >= FLY) {
			for (p = emitter; p < emitter + EMITTER_N && p->timer; p++)
				;
			if (p < emitter + EMITTER_N) {
				p->timer = esp->prm;
				p->nexttimer = 1;
				p->type = esp->type != RANDOM ? esp->type : FLY + R(diff / 13);
			}
		}
		else gentable[esp->type]();
	for (p = emitter; p < emitter + EMITTER_N; p++) {
		if (p->timer) {
			p->timer--;
			if (!--p->nexttimer) {
				u8 t = interval[p->type - FLY];
				p->nexttimer = t + R(t >> 1);
				gentable[p->type]();
			}
		}
	}
}
