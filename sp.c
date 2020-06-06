#include "lib8001.h"
#include "sp.h"
#include "game.h"
#include "mathi.h"
#include "bg.h"
#include "emitter.h"
#include "chr.h"
#include "pattern.h"
#include "music.h"

#define NOHIT			0
#define SPRITE_N		32
#define TARGET_LEN		45
#define EDGE			135
#define STATE_HIT		10
#define STATE_HIT_1ST	8
#define BLINKSPEED	3

#define HM_SB	1
#define HM_SPARIO	1
#define HM_ZAPPER	2
#define HM_FLY	3
#define HM_GROUND	0x30
#define HM_BLASTER	0x10
#define HM_LOCK	0x20

typedef struct {
	Sprite s;
	u8 type, state, timer;
	s8 sx, sy;
	u8 *data, *ptr;
} SpriteU;

static SpriteU sprite[SPRITE_N];

static SpriteU *AG, *SB;
static Sprite *targetHit;
static u8 boza, blinkframe, firstmsg;
static s8 blaster;

static SpriteU *create(u8 prio, Pattern *pat) {
	SpriteU *p = (SpriteU *)spriteCreate(prio, pat);
	if (!p) return nil;
	p->type = p->state = p->timer = 0;
	p->sx = p->sy = 0;
	p->data = p->ptr = nil;
	return p;
}

static u8 angleSB(Sprite *s) {
	return SB ? atni(SB->s.x - s->x >> PS, SB->s.y - s->y >> PS) : 0;
}

static void directSB(SpriteU *s, s8 shift) {
	u8 t = angleSB(&s->s); 
	s->s.speedX = cosi(t) >> shift;
	s->s.speedY = sini(t) >> shift;
}

//////// FLYING OBJECTS

static void spario(SpriteU *s, u8 n) {
	u8 t = 0;
	if (n != 16) {
		t = angleSB(&s->s);
		if (n == 5) t -= 0x20;
	}
	while (n--) {
		SpriteU *s1 = create(10, pat_spario);
		if (!s1) return;
		s1->s.x = s->s.x + (2 << PS);
		s1->s.y = s->s.y + (2 << PS);
		s1->s.speedX = cosi(t) >> 1;
		s1->s.speedY = sini(t) >> 1;
		s1->s.flags |= SF_LOOP;
		s1->s.hitMask = HM_SPARIO;
		t += 0x10;
	}
}

static u8 sparioFly(SpriteU *s, u8 prob) {
	s8 shift = 0x80 - diffGet() >> 5;
	if (shift >= 3) return 0;
	if (shift < 0) shift = 0;
	if (!R(prob << shift)) {
		spario(s, 1);
		return 1;
	}
	return 0;
}

static void crash(Sprite *p, u8 score) {
	playStart(4, MUSIC_CRASH, 0);
	spriteAnim(p, pat_crash);
	p->flags = SF_ERASE_NO_ANIM;
	p->speedX = p->speedY = 0;
	p->behavior = nil;
	scoreAdd(score);
	diffAddFrac(0x40);
}

//////// Bacura

static u8 behaviorBacura(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state == STATE_HIT) {
		s->state = 0;
		playStart(4, MUSIC_BACURA, 0);
	}
	return spriteVisible(p);
}

static void generateBacura() {
	SpriteU *s = create(10, pat_bacura);
	if (!s) return;
	s->s.behavior = behaviorBacura;
	s->s.x = EDGE << PS;
	s->s.y = R(84) << PS;
	s->s.speedX = -2 << PS;
	s->s.animSpeed = 8;
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_FLY;
}

//////// Giddo Spario

static u8 behaviorGiddoSpario(Sprite *p) {
	if (((SpriteU *)p)->state == STATE_HIT) crash(p, 1);
	return spriteVisible(p);
}

static void generateGiddoSpario() {
	SpriteU *s = create(10, pat_giddo);
	if (!s) return;
	s->s.behavior = behaviorGiddoSpario;
	s->s.x = EDGE << PS;
	s->s.y = R(96) << PS;
	directSB(s, 0);
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_FLY;
}

//////// Kapi / Terrazi

static u8 behaviorKT(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state || s->type) sparioFly(s, s->type ? 10 : 20);
	if (!s->state) {
		if (--s->timer < 8) {
			s16 m = s->timer;
			p->speedX = m * s->sx >> 3;
			p->speedY = m * s->sy >> 3;
			p->animSpeed = 8;
			if (!s->timer) {
				u8 t = R(0x40) - 0x20;
				s->sx = cosi(t);
				s->sy = sini(t);
				if (!s->type) {
					s->sx >>= 1;
					s->sy >>= 1;
				}
				s->state = 1;
			}
		}
	}
	else if (++s->timer <= 8) {
		s16 m = s->timer;
		p->speedX = m * s->sx >> 3;
		p->speedY = m * s->sy >> 3;
	}
	if (s->state == STATE_HIT) crash(p, s->type ? 70 : 30);
	return spriteVisible(p);
}

static void generateKT(u8 f) {
	SpriteU *s = create(10, f ? pat_terrazi : pat_kapi);
	if (!s) return;
	s->s.behavior = behaviorKT;
	s->s.x = EDGE << PS;
	s->s.y = R(92) << PS;
	directSB(s, f ? 0 : 1);
	s->sx = s->s.speedX;
	s->sy = s->s.speedY;
	s->s.animSpeed = 0;
	s->s.hitMask = HM_FLY;
	s->type = f;
	s->timer = f ? 5 + R(15) : 10 + R(30);
}

static void generateKapi() { generateKT(0); }
static void generateTerrazi() { generateKT(1); }

//////// Sheo Nite

static u8 behaviorSheoNite(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!SB) return 0;
	if (s->state == 2) return spriteVisible(p);
	s16 ox = 8 << PS, oy = (!s->state ? 8 : 8 - s->timer) << PS;
	if (s->type) oy = -oy;
	s16 dx = SB->s.x + ox - s->s.x >> PS;
	s16 dy = SB->s.y + oy - s->s.y >> PS;
	if (dx * dx + dy * dy < 16) {
		p->speedX = p->speedY = 0;
		p->x = SB->s.x + ox;
		p->y = SB->s.y + oy;
	}
	else {
		u8 t = atni(dx, dy);
		p->speedX = cosi(t) >> 1;
		p->speedY = sini(t) >> 1;
	}
	if (!s->state) {
		if (++s->timer < 100) return 1;
		s->state = 1;
		spriteAnim(p, s->type ? pat_sheo_tl : pat_sheo_tr);
		p->flags = 0;
		s->timer = 0;
	}
	else if (s->timer < 8) s->timer++;
	else {
		if (s->state && s->type) return 0;
		s->state = 2;
		spriteAnim(p, pat_sheo_r);
		p->animSpeed = 1 << AS;
		p->flags = SF_LOOP;
		p->speedX = 7 << PS;
		playStart(2, MUSIC_SHEO, 0);
	}
	return 1;
}

static void generateSheoNite(u8 f) {
	SpriteU *s = create(10, f ? pat_sheo_l : pat_sheo_r);
	if (!s) return;
	s->s.behavior = behaviorSheoNite;
	s->s.x = EDGE << PS;
	s->s.y = R(92) << PS;
	s->s.flags = SF_LOOP;
	s->type = f;
}

static void generateSheoNites() {
	generateSheoNite(0);
	generateSheoNite(1);
}

//////// Torkan

static u8 behaviorTorkan(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!s->state && !--s->timer) {
		s->state = 1;
		p->animSpeed = 1 << AS;
		p->speedX = p->speedY = 0;
		s->timer = 6;
		spario(s, 1);
	}
	else if (s->state == 1 && !--s->timer) {
		s->state = 2;
		directSB(s, 1);
		p->speedX = -p->speedX;
		p->speedY = -p->speedY;
	}
	if (s->state == STATE_HIT) crash(p, 5);
	return spriteVisible(p);
}

static void generateTorkan() {
	SpriteU *s = create(10, pat_torkan);
	if (!s) return;
	s->s.behavior = behaviorTorkan;
	s->s.x = EDGE << PS;
	s->s.y = R(92) << PS;
	directSB(s, 1);
	s->s.animSpeed = 0;
	s->s.hitMask = HM_FLY;
	s->timer = 5 + R(15);
}

//////// Toroid / Jara

static u8 behaviorTJ(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!SB) return 0;
	if (!s->state && p->speedY >= 0 ^ p->y < SB->s.y) {
		s->state = 1;
		p->animSpeed = 1 << AS;
		p->speedY = -p->speedY;
		sparioFly(s, 1);
	}
	else if (s->state) p->speedY += p->speedY >= 0 ? 2 : -2;
	if (s->state == STATE_HIT) crash(p, s->type ? 15 : 3);
	return spriteVisible(p);
}

static void generateTJ(u8 y, u8 f) {
	SpriteU *s = create(10, f ? pat_jara : pat_toroid);
	if (!s) return;
	s->s.behavior = behaviorTJ;
	s->s.x = EDGE << PS;
	s->s.y = y << PS;
	if (f) {
		directSB(s, 0);
		s->s.speedX = 5 * s->s.speedX >> 3;
		s->s.speedY = 5 * s->s.speedY >> 3;
	}
	else directSB(s, 2);
	s->s.animSpeed = 0;
	s->s.flags = SF_PALINDROME | SF_LOOP;
	s->s.hitMask = HM_FLY;
	s->type = f;
}

static void generateToroidGroup() { emitterGroup(generateTJ, 0); }
static void generateJaraGroup() { emitterGroup(generateTJ, 1); }

//////// Zakato / Brag Zakato / Garu Zakato

static u8 behaviorBragSpario(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	s16 dx = SB->s.x - p->x >> PS, dy = SB->s.y - p->y >> PS;
	u8 l = sqrti(dx * dx + dy * dy);
	if (!l) l = 1;
	dx = (dx << PS + 1) / l;
	dy = (dy << PS + 1) / l;
	p->speedX = s->sx + dx;
	p->speedY = s->sy + dy;
	return spriteVisible(p);
}

static void generateBragSpario(Sprite *p, s8 sx, s8 sy) {
	SpriteU *s = create(10, pat_bragspario);
	if (!s) return;
	s->s.behavior = behaviorBragSpario;
	s->s.x = p->x;
	s->s.y = p->y;
	s->sx = sx;
	s->sy = sy;
	s->s.hitMask = HM_FLY;
	s->s.flags = SF_LOOP;
}

static u8 behaviorZakato(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->timer && !--s->timer) {
		p->animSpeed = -1 << AS;
		if (s->type == 3) {
			spario(s, 16);
			u8 t = angleSB(p);
			s8 sx = cosi(t) >> 1, sy = sini(t) >> 1;
			generateBragSpario(p, sx + 16, sy);
			generateBragSpario(p, sx - 16, sy);
			generateBragSpario(p, sx, sy + 16);
			generateBragSpario(p, sx, sy - 16);
			playStart(2, MUSIC_GARUZAKATO, 0);
		}
		else spario(s, s->type == 2 ? 5 : 1);
	}
	if (s->state == STATE_HIT) crash(p, 10);
	return (s->timer || p->animSpeed) && spriteVisible(p);
}

static void generateZakato(u8 f) {
	SpriteU *s = create(10, f == 2 ? pat_brag : f == 3 ?  pat_garuzakato : pat_zakato);
	if (!s) return;
	s->s.behavior = behaviorZakato;
	s->s.x = (f == 3 ? EDGE : R(50) + 80) << PS;
	s->s.y = R(92) << PS;
	if (!f) s->s.speedX = -2 << PS;
	else if (f == 3) s->s.speedX = -4 << PS;
	else directSB(s, 1);
	s->s.hitMask = HM_FLY;
	s->type = f;
	s->timer = 5 + R(15);
}

static void generateZakatoS() { generateZakato(0); }
static void generateZakatoD() { generateZakato(1); }
static void generateBragZakato() { generateZakato(2); }
static void generateGaruZakato() { generateZakato(3); }

//////// Zoshi

static u8 behaviorZoshi(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	sparioFly(s, 50);
	if (!--s->timer) {
		s->timer = 20;
		if (!R(5)) {
			u8 t = R(256);
			s->s.speedX = cosi(t) >> 2;
			s->s.speedY = sini(t) >> 2;
		}
	}
	if (s->state == STATE_HIT) crash(p, 7);
	return spriteVisible(p);
}

static void generateZoshi(u8 f) {
	static u8 b;
	SpriteU *s = create(10, pat_zoshi);
	if (!s) return;
	s->s.behavior = behaviorZoshi;
	s->s.x = (f && b++ & 1 ? -6 : EDGE) << PS;
	s->s.y = R(92) << PS;
	directSB(s, 2);
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_FLY;
	s->timer = 20;
}

static void generateZoshiN() { generateZoshi(0); }
static void generateZoshiB() { generateZoshi(1); }

//////// GROUND OBJECTS

static u8 sparioGround(SpriteU *s, u8 prob) {
	s8 shift = EDGE - (s->s.x >> PS) >> 6;
	if (shift < 0) shift = 0;
	if (!R(prob << shift)) {
		spario(s, 1);
		return 1;
	}
	return 0;
}

static u8 behaviorBomb(Sprite *p) {
	if (!p->animSpeed) {
		spriteAnim(p, pat_burn);
		p->flags = SF_LOOP;
		p->x += 8 << PS;
		p->y += 8 << PS;
		p->behavior = nil;
		if (((SpriteU *)p)->type == BOZALOGRAM) boza = 0;
	}
	return spriteVisible(p);
}

static void bomb(Sprite *p, u16 score, u8 burn) {
	playStart(5, MUSIC_BOMB, 0);
	if (burn) p->behavior = behaviorBomb;
	else p->behavior = nil;
	if (p->pat != pat_sol) p->x -= 8 << PS;
	p->y -= 8 << PS;
	p->speedX = -1 << PS;
	p->speedY = 0;
	spriteAnim(p, pat_bomb);
	p->flags = SF_PALINDROME | (burn ? 0 : SF_ERASE_NO_ANIM);
	scoreAdd(score);
}

//////// Barra / Derota / Zolbak

static u8 behaviorBDZ(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->type == 1) sparioGround(s, 20);
	if (s->state == STATE_HIT) {
		if (s->type == 2) diffAdd(-2);
		bomb(p, s->type == 0 ? 10 : s->type == 1 ? 100 : 20, 1);
	}
	return spriteVisible(p);
}

static void generateBDZ(u8 f) {
	SpriteU *s = create(0, f == 0 ? pat_barra : f == 1 ? pat_derota : pat_zolbak);
	if (!s) return;
	s->s.behavior = behaviorBDZ;
	s->s.x = EDGE << PS;
	s->s.y = emitterPrm() << PS;
	s->s.speedX = -1 << PS;
	spriteFrame(&s->s, blinkframe);
	s->s.animSpeed = BLINKSPEED;
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_GROUND;
	s->type = f;
}

static void generateBarra() { generateBDZ(0); }
static void generateDerota() { generateBDZ(1); }
static void generateZolbak() { generateBDZ(2); }

//////// Garu Barra / Garu Derota

static u8 behaviorGBD(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->type == 1) sparioGround(s, 10);
	if (s->state == STATE_HIT) bomb(p, s->type ? 200 : 30, 0);
	return p->x >= EDGE << PS || spriteVisible(p);
}

static void generateGBD(u8 f) {
	SpriteU *s = create(0, f ? pat_garuderota : pat_garubarra);
	if (!s) return;
	s->s.x = EDGE << PS;
	s->s.y = emitterPrm() << PS;
	s->s.speedX = -1 << PS;
	s->s.animSpeed = 4;
	s->s.flags = SF_LOOP;
	spriteFrame(&s->s, blinkframe);
	s->s.animSpeed = BLINKSPEED;
	s->s.flags = SF_LOOP;
	s = create(0, f ? pat_garuderota_t : pat_garubarra_t);
	if (!s) return;
	s->s.behavior = behaviorGBD;
	s->s.x = EDGE + (f ? 4 : 3) << PS;
	s->s.y = emitterPrm() + 4 << PS;
	s->s.speedX = -1 << PS;
	spriteFrame(&s->s, blinkframe);
	s->s.animSpeed = BLINKSPEED;
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_GROUND;
	s->type = f;
}

static void generateGaruBarra() { generateGBD(0); }
static void generateGaruDerota() { generateGBD(1); }

//////// Domogram / Grobda

extern u8 extradata[];
extern u16 extraofs[];

static u8 behaviorDG(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!s->type && sparioGround(s, 30)) {
		p->animSpeed = 1 << AS;
		p->flags = SF_PALINDROME;
	}
	if (s->type ? !s->state || blaster && p == targetHit : !s->timer) {
		if (!*s->ptr) s->ptr = &s->data[s->state == 1 && *s->data ?  2 : 0];
		if (s->type && !s->state) s->state = 1;
		if (*s->ptr) { // recheck in case no movement
			s->timer = *s->ptr++;
			s8 d = *s->ptr++;
			if (s->type) p->animSpeed = d ? 1 << AS : 0;
			p->speedX = (d >> 4 << PS - 2) - (1 << PS);
			d <<= 4;
			p->speedY = d >> 4 << PS - 2;
		}
	}
	if (s->timer && !--s->timer && s->type == 1) {
		p->speedX = -1 << PS;
		p->speedY = 0;
		p->animSpeed = 0;
	}
	if (s->state == STATE_HIT) {
		u16 score = 80;
		if (s->type) {
			score = s->data[-1];
			score = score << 8 | s->data[-2];
		}
		u8 stage = bgStage();
		bomb(p, score, !s->type || stage != 5 && stage != 13);
	}
	return p->x >= EDGE << PS || spriteVisible(p);
}

static void generateDG(u8 f) {
	SpriteU *s = create(0, f ? pat_grobda : pat_domogram);
	if (!s) return;
	s->s.behavior = behaviorDG;
	u8 *ptr = &extradata[extraofs[emitterPrm()]];
	s->s.x = EDGE << PS;
	s->s.y = *ptr++ << PS;
	s->data = s->ptr = f ? ptr + 2 : ptr;
	s->s.speedX = -2 << PS;
	s->s.animSpeed = 0;
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_GROUND;
	s->type = f;
	if (f) s->s.speedX = -1 << PS;
}

static void generateDomogram() { generateDG(0); }
static void generateGrobda() { generateDG(1); }

//////// Logram / Boza Logram

static u8 behaviorLogram(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->timer) s->timer--;
	else if (sparioGround(s, 50)) {
		p->animSpeed = 1 << AS;
		s->timer = 10;
	}
	if (s->state == STATE_HIT || s->type == 1 && boza) bomb(p, 30, 1);
	return p->x >= EDGE << PS || spriteVisible(p);
}

static void generateLogram1(s8 ofsx, s8 ofsy, u8 f) {
	SpriteU *s = create(0, pat_logram);
	if (!s) return;
	s->s.behavior = behaviorLogram;
	s->s.x = EDGE + ofsx << PS;
	s->s.y = emitterPrm() + ofsy << PS;
	s->s.speedX = -1 << PS;
	s->s.animSpeed = 0;
	s->s.flags = SF_PALINDROME;
	s->s.hitMask = HM_GROUND;
	s->type = f;
}

static void generateLogram() { generateLogram1(0, 0, 0); }

static u8 behaviorBoza(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state == STATE_HIT) {
		boza = 1;
		bomb(p, 80, 1);
	}
	return p->x >= EDGE << PS || spriteVisible(p);
}

static void generateBozaLogram() {
	SpriteU *s = create(0, pat_boza);
	if (!s) return;
	s->s.behavior = behaviorBoza;
	s->s.x = EDGE + 7 << PS;
	s->s.y = emitterPrm() << PS;
	s->s.speedX = -1 << PS;
	s->s.hitMask = HM_GROUND;
	s->type = BOZALOGRAM;
	generateLogram1(14, 0, 1);
	generateLogram1(7, 7, 1);
	generateLogram1(7, -7, 1);
	generateLogram1(0, 0, 1);
}

//////// Special

static u8 behaviorSpecial(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state == STATE_HIT_1ST) {
		s->state++;
		p->animSpeed = 1 << AS;
		p->hitMask = HM_SB;
		playStart(5, MUSIC_BOMB, 0);
	}
	else if (s->state == STATE_HIT) {
		playStart(5, MUSIC_SPECIAL, 0);
		return 0;
	}
	return spriteVisible(p);
}

static void generateSpecial() {
	SpriteU *s = create(0, pat_special);
	if (!s) return;
	s->s.behavior = behaviorSpecial;
	s->s.x = EDGE << PS;
	s->s.y = R(92) << PS;
	s->s.speedX = -1 << PS;
	s->s.animSpeed = 0;
	s->s.hitMask = HM_GROUND;
}

//////// Sol

static u8 behaviorSol(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state == STATE_HIT_1ST) {
		s->state++;
		p->animSpeed = 4;
		scoreAdd(200);
		playStart(5, MUSIC_BOMB, 0);
	}
	else if (s->state == STATE_HIT) bomb(p, 200, 1);
	return spriteVisible(p);
}

static void generateSol() {
	SpriteU *s = create(0, pat_sol);
	if (!s) return;
	s->s.behavior = behaviorSol;
	s->s.x = EDGE << PS;
	s->s.y = emitterPrm() << PS;
	s->s.speedX = -1 << PS;
	s->s.animSpeed = 0;
	s->s.hitMask = HM_GROUND;
	s->s.insetLeft = s->s.insetBottom = 8;
}

//////// Andore Genesis

static u8 behaviorAndoreGenesis(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (s->state == 0 && ++s->timer >= 25) {
		s->state = 1;
		p->speedX = 0;
		s->timer = 0;
	}
	else if (s->state == 1 && ++s->timer >= s->type) {
		s->state = 2;
		p->speedX = 1 << PS;
	}
	else if (s->state == 2 && !spriteVisible(p)) {
		playStop(1);
		return 0;
	}
	if (s->state == STATE_HIT) {
		s->s.speedX = -1 << PS;
		AG = nil;
	}
	return spriteVisible(p);
}

static u8 behaviorAndoreGenesisAlgo(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!AG) return 0;
	s->s.speedX = AG->s.speedX;
	sparioGround(s, 30);
	if (s->state == STATE_HIT) bomb(p, 100, 0);
	return !AG->state || spriteVisible(p);
}

static u8 behaviorAndoreGenesisCore(Sprite *p) {
	SpriteU *s = (SpriteU *)p;
	if (!AG) return 0;
	s->s.speedX = AG->s.speedX;
	if (s->state == STATE_HIT) {
		AG->state = STATE_HIT;
		s = create(10, pat_bragza);
		if (!s) return 0;
		s->s.x = p->x;
		s->s.y = p->y;
		s->s.speedX = 7 << PS;
		playStop(1);
		bomb(p, 400, 0);
		return 0;
	}
	return !AG->state || spriteVisible(p);
}

static void generateAndoreGenesis() {
	SpriteU *s = create(1, pat_core);
	if (!s) return;
	s->s.behavior = behaviorAndoreGenesisCore;
	s->s.x = EDGE + 19 << PS;
	s->s.y = 32 + 15 << PS;
	s->s.speedX = -3 << PS;
	s->s.flags = SF_LOOP;
	s->s.animSpeed = 4;
	s->s.hitMask = HM_GROUND;
	for (u8 i = 0; i < 4; i++) {
		s = create(1, (i ^ i >> 1) & 1 ? pat_algoa : pat_algob);
		if (!s) return;
		s->s.behavior = behaviorAndoreGenesisAlgo;
		s->s.x = EDGE + (i & 1 ? 27 : 11) << PS;
		s->s.y = 32 + (i & 2 ? 23 : 7) << PS;
		s->s.speedX = -3 << PS;
		s->s.flags = SF_LOOP;
		s->s.animSpeed = 4;
		s->s.hitMask = HM_GROUND;
	}
	s = create(0, pat_ag);
	if (!s) return;
	s->s.behavior = behaviorAndoreGenesis;
	s->s.x = EDGE << PS;
	s->s.y = 28 << PS;
	s->s.speedX = -3 << PS;
	s->type = emitterPrm();
	AG = s;
	playStart(1, MUSIC_AG, PF_LOOP);
}

//////// Solvalou / Target / Zapper / Blaster

static u8 behaviorZapper(Sprite *p) {
	Sprite *h = spriteHit(p);
	if (h) {
		((SpriteU *)h)->state = STATE_HIT;
		return 0;
	}
	return spriteVisible(p);
}

static u8 behaviorBlaster(Sprite *p) {
	if (++((SpriteU *)p)->timer < 10) return 1;
	blaster = -1;
	return 0;
}

static void generateZapper() {
	SpriteU *s = create(10, pat_zapper);
	if (!s || !SB) return;
	s->s.behavior = behaviorZapper;
	s->s.x = SB->s.x + (10 << PS);
	s->s.y = SB->s.y;
	s->s.speedX = 7 << PS;
	s->s.hitMask = HM_ZAPPER;
	playStart(3, MUSIC_ZAP, 0);
}

static void generateBlaster() {
	SpriteU *s1 = create(10, pat_blaster);
	if (!s1 || !SB) return;
	s1->s.behavior = behaviorBlaster;
	s1->s.x = SB->s.x + (6 << PS);
	s1->s.y = SB->s.y + (2 << PS);
	s1->s.speedX = 4 << PS;
	s1->s.animSpeed = 4;
	s1->s.hitMask = HM_BLASTER;
	blaster = 1;
	playStart(2, MUSIC_BLASTER, 0);
}

static u8 behaviorTarget(Sprite *p) {
	if (!SB) return 0;
	if (SB->s.pat == pat_solvalou) {
		p->x = SB->s.x + (TARGET_LEN << PS);
		p->y = SB->s.y;
	}
	return 1;
}

static u8 behaviorTargetLock(Sprite *p) {
	if (!SB) return 0;
	Sprite *list[4];
	u8 n = spriteHitArray(p, list);
	p->speedX = blaster ? -1 << PS : 0;
	if (SB->s.pat == pat_solvalou && blaster <= 0) {
		p->x = SB->s.x + (TARGET_LEN << PS);
		p->y = SB->s.y;
	}
	if (blaster < 0) {
		blaster = 0;
		for (u8 i = 0; i < n; i++) {
			SpriteU *h = (SpriteU *)list[i];
			if (h->s.pat == pat_sol || h->s.pat == pat_special) {
				if (h->state < STATE_HIT_1ST) h->state = STATE_HIT_1ST;
				else if (!h->s.animSpeed) h->state = STATE_HIT;
			}
			else if (h->state < STATE_HIT) h->state = STATE_HIT;
		}
		if (SB && SB->s.y >= 90 << PS && bgOfs() < 0 && firstmsg < 7 && ++firstmsg == 7) {
			chrTime(40);
			chrLocate(2, 36);
			chrPuts("PC8001 PORT YK");
			scoreAdd(1);
			playStart(5, MUSIC_BOMB, 0);
		}
	}
	if (n && list[0]->pat != pat_special) p->animSpeed = 8;
	else p->frame = p->animSpeed = 0;
	return 1;
}

static u8 behaviorSolvalouDead(Sprite *p) {
	((SpriteU *)p)->timer++;
	if (!p->animSpeed) {
		p->x = 0;
		p->y = 100 << PS;
	}
	return 1;
}

static u8 behaviorSolvalou(Sprite *p) {
	static u8 d, demotimer, lastz;
	if (!isDemo()) d = KEY0 & 0x54 | KEY1 & 1 | triggerGet() << 1 & 0xa;
	else if (++demotimer >= 10) {
		demotimer = 0;
		d = R(0x100);
	}
	if (p->x >= 2 << PS && !(d & 4)) p->x -= 2 << PS;
	if (p->x <= 80 << PS && !(d & 1)) p->x += 2 << PS;
	if (p->y >= 2 << PS && !(d & 0x10)) p->y -= 2 << PS;
	if (p->y <= 90 << PS && !(d & 0x40)) p->y += 2 << PS;
	if (!blaster && !(d & 2)) generateBlaster();
	u8 z = d & 8;
	if (!z && lastz && spriteCount(pat_zapper) < 3) generateZapper();
	lastz = z;
	Sprite *h = spriteHit(p);
	if (h->pat == pat_special) {
		((SpriteU *)h)->state = STATE_HIT;
		reserveInc();
	}
	else {
#if NOHIT
		p->frame = h ? 1 << AS : 0;
#else
		SpriteU *s = (SpriteU *)p;
		if (h) {
			spriteAnim(p, pat_bomb);
			p->flags = SF_PALINDROME;
			p->x -= 8 << PS;
			p->y -= 8 << PS;
			playStart(5, MUSIC_BOMB, 0);
			p->behavior = behaviorSolvalouDead;
			diffAdd(-16);
		}
#endif
	}
	return 1;
}

static void generateSolvalou() {
	SpriteU *s = create(10, pat_solvalou);
	if (!s) return;
	s->s.behavior = behaviorSolvalou;
	s->s.x = 0 << PS;
	s->s.y = 46 << PS;
	s->s.animSpeed = 0;
	s->s.hitMask = HM_SB;
	s->s.insetLeft = s->s.insetRight = s->s.insetTop = s->s.insetBottom = 3;
	SB = s;
	s = create(10, pat_target);
	if (!s) return;
	s->s.behavior = behaviorTarget;
	s->s.x = TARGET_LEN << PS;
	s->s.y = 46 << PS;
	s = create(10, pat_lock);
	if (!s) return;
	s->s.behavior = behaviorTargetLock;
	s->s.x = TARGET_LEN << PS;
	s->s.y = 46 << PS;
	s->s.speedX = -1 << PS;
	s->s.flags = SF_LOOP;
	s->s.hitMask = HM_LOCK;
	s->s.insetLeft = s->s.insetRight = s->s.insetTop = s->s.insetBottom = 2;
}

GenTable gentable = {
	nil,
	generateBarra,
	generateGaruBarra,
	generateZolbak,
	generateLogram,
	generateBozaLogram,
	generateDomogram,
	generateGrobda,
	generateDerota,
	generateGaruDerota,
	generateSol,
	generateSpecial,
	generateAndoreGenesis,
	generateToroidGroup,
	generateTorkan,
	generateZoshiN,
	generateGiddoSpario,
	generateJaraGroup,
	generateKapi,
	generateTerrazi,
	generateZakatoD,
	generateBragZakato,
	generateZakatoS,
	generateGaruZakato,
	generateBacura,
	generateZoshiB,
	generateSheoNites,
};

void spInit() {
	AG = SB = nil;
	targetHit = nil;
	boza = firstmsg = 0;
	blaster = 0;
	spriteSetupArray(sprite);
	spriteView(2, 0, 68, 25);
	generateSolvalou();
}

u8 spUpdate() {
	blinkframe = blinkframe + BLINKSPEED & (1 << AS + 1) - 1;
	return SB ? SB->timer < 40 : 0;
}
