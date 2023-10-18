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
#include "CommonConfig.h"
#include "wdvd.h"

#define EXECUTE_ADDR	((void *)0x92000000)
#define BOOTER_ADDR		((void *)0x92F00000)
#define ARGS_ADDR		((struct __argv *)0x93300800)
#define entry 			((void *)0x92F00000)

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

	const char *fPath = "sd:/apps/nintendont/boot.dol";
	FILE *f = fopen(fPath,"rb");
	if(!f)
	{
		nPrintf("boot.dol not found!\n");
		unmountSD();
		return -1;
	}
	fseek(f,0,SEEK_END);
	size_t fsize = ftell(f);
	fseek(f,0,SEEK_SET);
	fread(EXECUTE_ADDR,1,fsize,f);
	DCFlushRange(EXECUTE_ADDR,fsize);
	fclose(f);

	memcpy(BOOTER_ADDR,app_booter_bin,app_booter_bin_size);
	DCFlushRange(BOOTER_ADDR,app_booter_bin_size);

	fsize = 0;
	if(WDVD_Init() == 0)
	{
		if(WDVD_FST_Mount())
		{
			fsize = getIdFromIso();
			if(!fsize)
				WDVD_FST_Unmount();
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
	char *fPath2 = "sd:/nintendont/configs/XXXX.bin";
	*(uint32_t *)(fPath2 + strlen("sd:/nintendont/configs/")) = fsize;
	f = fopen(fPath2,"rb");
	if(f)
	{
		if(fread(&nincfg,sizeof(NIN_CFG),1,f) == 1)
		{
			debugPrint("%s loaded!\n", fPath2);
			goto clearNincfg;
		}

		debugPrint("Error reading %s!\n", fPath2);
		fclose(f);
	}
	else
		debugPrint("Error opening %s!\n", fPath2);

	fsize = 0;
	if(!fsize)
	{
		if(WDVD_FST_Open("nincfg.bin") == 0)
		{
			if(WDVD_FST_Read((uint8_t *)&nincfg,sizeof(NIN_CFG)) == sizeof(NIN_CFG))
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
	}
	else
		fsize = 0;

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
	unmountSD();

	char *CMD_ADDR = (char*)ARGS_ADDR + sizeof(struct __argv);
	fsize = strlen(fPath) + 1;
	size_t fsize2 = sizeof(NIN_CFG) + fsize;
	size_t full_args_len = sizeof(struct __argv)+fsize2;

	memset(ARGS_ADDR, 0, full_args_len);
	ARGS_ADDR->argvMagic = ARGV_MAGIC;
	ARGS_ADDR->commandLine = CMD_ADDR;
	ARGS_ADDR->length = fsize2;
	ARGS_ADDR->argc = 2;

	memcpy(CMD_ADDR, fPath, fsize);
	memcpy(CMD_ADDR+fsize, &nincfg, sizeof(NIN_CFG));
	DCFlushRange(ARGS_ADDR, full_args_len);

	deinitGraphics();

	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	__lwp_thread_stopmultitasking(entry);

	return 0;
}
