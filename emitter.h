#include "types.h"

enum {
	DIFF,
	BARRA, GARUBARRA, ZOLBAK, LOGRAM, BOZALOGRAM, DOMOGRAM, GROBDA, 
	DEROTA, GARUDEROTA, SOL, SPECIAL, GENESIS,
	FLY,
	TOROID = FLY, TORKAN, ZOSHI, GIDDOSPARIO, JARA, KAPI, TERRAZI, 
	ZAKATO_D, BRAGZAKATO, ZAKATO_S, GARUZAKATO,
	BACURA, ZOSHI_B, SHEONITE,
	RANDOM
};

typedef struct {
	s16 ofs, prm;
	u8 type;
} EmitterSch;

typedef void (*GenTable[])(void);
typedef void (*GroupFunc)(u8 y, u8 u);

void emitterInit(void);
void emitterStart(void);
void emitterGroup(GroupFunc func, u8 u);
s16 emitterPrm(void);
void emitterUpdate(void);

void diffAdd(s8 v);
void diffAddFrac(u8 v);
u8 diffGet(void);
