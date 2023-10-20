/*
 * Copyright (C) 2017 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fat.h>
#include <gccore.h>
#include <ogc/lwp_threads.h>
#include <sdcard/wiisd_io.h>

#include "app_booter_bin.h"
#include "common.h"
#include "CommonConfig.h"
#include "wdvd.h"

static GXRModeObj *rmode = NULL;

static bool __attribute__((noinline)) initGraphics()
{
	if(rmode)
		return true;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	if(!rmode)
		return false;

	void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	if(!xfb)
	{
		rmode = NULL;
		return false;
	}

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	CON_InitEx(rmode, 24, 32, rmode->fbWidth - 32, rmode->xfbHeight - 48);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	printf(" \n");
	return true;
}

static void nPrintf(const char *msg, ...)
{
	if(initGraphics())
	{
		va_list args;
		va_start(args, msg);
		vprintf(msg, args);
		va_end(args);
		sleep(2);
	}
}

#ifdef DEBUG_BUILD
	#define debugPrint(...) nPrintf(__VA_ARGS__)
static inline void deinitGraphics()
{
	if(!rmode)
		return;

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}
#else
	#define debugPrint(...)
	#define deinitGraphics()
#endif

static uint32_t getIdFromIso()
{
	uint32_t ret = 0;
	if(WDVD_FST_OpenDisc(0) == 0)
	{
		if(WDVD_FST_Read((uint8_t *)&ret, 4) != 4)
		{
			ret = 0;
			debugPrint("Error reading iso!\n");
		}

		WDVD_FST_Close();
	}
	else
		debugPrint("Error opening iso!\n");

	return ret;
}

static inline void unmountSD()
{
	fatUnmount("sd:");
	__io_wiisd.shutdown();
}

static inline void unmountISO()
{
	WDVD_FST_Unmount();
	WDVD_Close();
}

int main(int argc, char *argv[]) 
{
	debugPrint("Hello world!\n");

	__io_wiisd.startup();
	__io_wiisd.isInserted();
	fatMount("sd", &__io_wiisd, 0, 4, 64);

	size_t fsize = 0;
	if(WDVD_Init() == 0)
	{
		if(WDVD_FST_Mount())
		{
			fsize = getIdFromIso();
			if(!fsize)
				unmountISO();
		}
		else
		{
			debugPrint("Error mounting iso!\n");
			WDVD_Close();
		}
	}
	else
		debugPrint("Error initialising WDVD!\n");

	if(!fsize)
	{
		unmountSD();
		return -2;
	}

	NIN_CFG nincfg;
	char *fPath = "sd:/nintendont/configs/XXXX.bin";
	*(uint32_t *)(fPath + strlen("sd:/nintendont/configs/")) = fsize;
	FILE *f = fopen(fPath,"rb");
	if(f)
	{
		if(fread(&nincfg,sizeof(NIN_CFG),1,f) == 1)
		{
			debugPrint("%s loaded!\n", fPath);
			goto clearNincfg;
		}

		debugPrint("Error reading %s!\n", fPath);
		fclose(f);
	}
	else
		debugPrint("Error opening %s!\n", fPath);

	fsize = 0;
	if(WDVD_FST_Open("nincfg.bin") == 0)
	{
		if(WDVD_FST_Read((uint8_t *)&nincfg, sizeof(NIN_CFG)) == sizeof(NIN_CFG))
		{
			debugPrint("Nincfg embedded into inject loaded!\n");
			fsize = 1;
		}
		else
			debugPrint("Error reading nincfg from iso!\n");

		WDVD_FST_Close();
	}
	else
		debugPrint("Error opening nincfg from iso!\n");

	if(!fsize)
	{
		f = fopen("sd:/nincfg.bin","rb");
		if(!f)
		{
			unmountISO();
			unmountSD();
			nPrintf("Error opening nincfg!\n");
			return -3;
		}

		if(fread(&nincfg,sizeof(NIN_CFG),1,f) != 1)
		{
			fclose(f);
			unmountISO();
			unmountSD();
			nPrintf("Error reading nincfg!\n");
			return -4;
		}

		debugPrint("Fallback nincfg loaded!\n");

	clearNincfg:
		fclose(f);
		nincfg.CheatPath[0] = '\0';
		nincfg.Config |= (NIN_CFG_AUTO_BOOT);
		nincfg.Config &= ~(NIN_CFG_USB);
		strcpy(nincfg.GamePath,"di");
	}

	unmountISO();

	f = fopen("sd:/apps/nintendont/boot.dol","rb");
	if(!f)
	{
		nPrintf("boot.dol not found!\n");
		unmountSD();
		return -5;
	}

	fseek(f,0,SEEK_END);
	fsize = ftell(f);
	fseek(f,0,SEEK_SET);
	if(fread(EXECUTE_ADDR,fsize,1,f) != 1)
	{
		nPrintf("Couldn't read boot.dol!\n");
		fclose(f);
		unmountSD();
		return -6;
	}
	DCFlushRange(EXECUTE_ADDR,fsize);
	fclose(f);
	unmountSD();

	memset(ARGS_ADDR, 0, sizeof(struct __argv) + 1);
	ARGS_ADDR->argvMagic = ARGV_MAGIC;
	ARGS_ADDR->commandLine = (char*)ARGS_ADDR + sizeof(struct __argv);
	ARGS_ADDR->length = sizeof(NIN_CFG) + 1;
	ARGS_ADDR->argc = 2;

	memcpy((char*)ARGS_ADDR + sizeof(struct __argv) + 1, &nincfg, sizeof(NIN_CFG));
	DCFlushRange(ARGS_ADDR, sizeof(struct __argv) + sizeof(NIN_CFG) + 1);

	memcpy(BOOTER_ADDR,app_booter_bin,app_booter_bin_size);
	DCFlushRange(BOOTER_ADDR,app_booter_bin_size);

	deinitGraphics();

	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	__lwp_thread_stopmultitasking(ENTRY_ADDR);

	return 0;
}
