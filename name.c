#include "lib8001.h"
#include "name.h"
#include "chr.h"
#include "game.h"
#include "music.h"
#include <string.h>

typedef struct {
	u16 score;
	u8 name[9];
} Score;

static Score best[5] = {
	{ 4000, "NAKAMURA" },
	{ 3500, "EIRRY MO" },
	{ 3000, "EVZO END" },
	{ 2500, "OKAMOTO " },
	{ 2000, "S KOJIMA" },
};
static u8 pos;
static s8 edit = -1;

static void record(u8 index, u8 color) {
	chrInit();
	chrTime(1);
	chrColor(color);
	Score *p = &best[index];
	chrLocate(2, 25 + 3 * index);
	u8 buf[6];
	chrValue(buf, sizeof(buf), p->score);
	chrPuts(buf);
	chrPuts("0 ");
	chrPutsBuf(p->name);
	chrUpdate();
}

u8 nameEntry(u16 score) {
	s8 i;
	for (i = 0; i < 5 && score <= best[i].score; i++)
		;
	if (i < 5) {
		memmove(&best[i + 1], &best[i], sizeof(Score) * (4 - i));
		best[i].score = score;
		strcpy(best[i].name, "A       ");
		edit = i;
		playStart(5, i ? MUSIC_NAME : MUSIC_BEST, PF_LOOP);
	}
	else edit = -1;
	return edit >= 0;
}

static void nameInit() {
	pos = 0;
	chrTime(1);
	if (edit >= 0) {
		chrColor(2);
		chrLocate(0, 12);
		chrPuts("CONGRATURATIONS!");
		chrFlush();
		chrColor(2);
		chrLocate(1, 18);
		chrPuts("ENTER YOUR NAME");
	}
	else {
		logoInit();
		logoUpdate();
		chrColor(0);
		chrLocate(1, 20);
		chrPuts("BEST 5 WARRIORS");
	}
	chrFlush();
	for (u8 i = 0; i < 5; i++) record(i, 0);
	if (edit >= 0) record(edit, 6);
}

static u8 nameUpdate() {
	u8 c = keyDown(1);
	if (edit >= 0) {
		u8 *name = best[edit].name;
		if (c == '4' && --name[pos] < '@') name[pos] = 'Z';
		if (c == '6' && ++name[pos] > 'Z') name[pos] = '@';
		if (c == 'z' && pos > 0) name[pos--] = '@';
		if (c == 'x')
			if (pos < 7) name[++pos] = 'A';
			else {
				playStopAll();
				return 0;
			}
	}
	else if (c == 'x') return 0;
	return 1;
}

void nameMain() {
	vramSingle();
	cls();
	chrInit();
	scorePrint();
	chrFlush();
	nameInit();
	u8 t = 0;
	while (nameUpdate()) {
		if (isDemo() && ++t >= 100) return;
		scoreUpdate();
		chrUpdate();
		waitVSync(3);
	}
	demoSet(0);
}
