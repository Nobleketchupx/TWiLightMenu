#include "bootsplash.h"
#include <nds.h>
#include <maxmod9.h>

#include "common/twlmenusettings.h"
#include "common/flashcard.h"
#include "common/systemdetails.h"
#include "common/tonccpy.h"
#include "graphics/gif.hpp"
#include "graphics/graphics.h"
#include "common/lodepng.h"

#include "sound.h"

extern bool useTwlCfg;

extern bool fadeType;
extern bool controlTopBright;
extern bool controlBottomBright;
extern int screenBrightness;

//extern void loadROMselectAsynch(void);

bool cartInserted;

void bootSplashDSi(void) {

	cartInserted = (REG_SCFG_MC != 0x11);

	int language = ms().gameLanguage;
	if (ms().gameLanguage == -1) {
		language = (useTwlCfg ? *(u8*)0x02000406 : PersonalData->language);
	}

	char currentDate[16];
	time_t Raw;
	time(&Raw);
	const struct tm *Time = localtime(&Raw);

	strftime(currentDate, sizeof(currentDate), "%m/%d", Time);
	const bool virtualPain = (ms().dsiSplashEasterEggs && (strcmp(currentDate, "04/01") == 0 || strcmp(currentDate, ms().getGameRegion() == 0 ? "07/21" : "08/14") == 0));
	const bool super = (*(u16*)(0x020000C0) == 0x334D || *(u16*)(0x020000C0) == 0x3647 || *(u16*)(0x020000C0) == 0x4353);
	const bool regularDS = (sys().isRegularDS() && !ms().oppositeSplash) || (!sys().isRegularDS() && ms().oppositeSplash);

	const bool custom = ms().dsiSplash == 3;

	char path[256];
	if (virtualPain) {
		sprintf(path, "nitro:/video/splash/virtualPain%s.gif", "");
	} else if (custom && access("/_nds/TWiLightMenu/extras/splashtop.gif", F_OK) == 0) {
		sprintf(path, "%s:/_nds/TWiLightMenu/extras/splashtop.gif", sys().isRunFromSD() ? "sd" : "fat");
	} else if (ms().macroMode) {
		sprintf(path, "nitro:/video/splash/gameBoy.gif");
	} else if (super) {
		sprintf(path, "nitro:/video/splash/superDS.gif");
	} else {
		sprintf(path, "nitro:/video/splash/%s.gif", language == TWLSettings::ELangChineseS ? "iquedsi" : (regularDS ? "ds" : "dsi"));
	}
	Gif splash(path, true, true);

	path[0] = '\0';
	if (virtualPain) {
		strcpy(path, "nitro:/video/hsmsg/virtualPain.gif");
	} else if (ms().dsiSplash == 1) {
		sprintf(path, "nitro:/video/tttstc/%i.gif", language);
	} else if (ms().dsiSplash == 2) {
		sprintf(path, "nitro:/video/hsmsg/%i.gif", language);
	} else if (custom && access("/_nds/TWiLightMenu/extras/splashbottom.gif", F_OK) == 0) {
		sprintf(path, "%s:/_nds/TWiLightMenu/extras/splashbottom.gif", sys().isRunFromSD() ? "sd" : "fat");
	}
	Gif healthSafety(path, false, true);

	if (!custom) {
		healthSafety.displayFrame();
		healthSafety.pause();
	}

	//  FIX: move GIF to TIMER1 (avoids conflict with audio)
	timerStart(1, ClockDivider_1024, TIMER_FREQ_1024(100), Gif::timerHandler);

	controlBottomBright = true;
	fadeType = true;

	//  Track if music started
	static bool musicStarted = false;

	if ((!custom && ms().macroMode) || (splash.loopForever() && healthSafety.loopForever())) {
		for (int i = 0; i < 60 * 3 && !keysDown(); i++) {
			swiWaitForVBlank();
			scanKeys();

			// Start custom music once
			if (custom && !musicStarted && splash.currentFrame() >= 1) {
				snd().beginStream();
				musicStarted = true;
			}
		}
	} else {
		u16 pressed = 0;
		while (!(splash.finished() && healthSafety.finished()) && !(pressed & KEY_START)) {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			pressed &= ~KEY_LID;

			//  Start custom music once
			if (custom && !musicStarted && splash.currentFrame() >= 2) {
				snd().beginStream();
				musicStarted = true;
			}

			if (splash.waitingForInput()) {
				if (!custom && healthSafety.paused() && !ms().dsiSplashAutoSkip)
					healthSafety.unpause();
				if (pressed || ms().dsiSplashAutoSkip) {
					splash.resume();
					if (!ms().dsiSplashAutoSkip) snd().playSelect();
				}
			}

			if (healthSafety.waitingForInput()) {
				if (pressed || ms().dsiSplashAutoSkip) {
					healthSafety.resume();
					if (!ms().dsiSplashAutoSkip) snd().playSelect();
				}
			}

			if (!custom && splash.currentFrame() == (super ? 1 : 26))
				snd().playDSiBoot();
		}
	}

	// Fade out
	controlTopBright = true;
	controlBottomBright = true;
	fadeType = false;
	for (int i = 0; i < 25; i++) { swiWaitForVBlank(); }

	//  STOP audio cleanly (prevents crash)
	if (custom) {
		snd().stopStream();
	}

	// FIX: match timerStart(1)
	timerStop(1);
}
void bootSplashInit(void) {
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG);

	bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
	bgSetPriority(3, 3);

	bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
	bgSetPriority(7, 3);

	oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);

	snd();
	bootSplashDSi();
}
