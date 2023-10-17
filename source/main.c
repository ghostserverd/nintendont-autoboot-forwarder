/*
 * Copyright (C) 2017 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <fat.h>
#include <string.h>
#include <unistd.h>
#include <ogc/lwp_threads.h>
#include <sdcard/wiisd_io.h>
#include "CommonConfig.h"
#include "app_booter_bin.h"
#include "wdvd.h"

static u8 *EXECUTE_ADDR = (u8*)0x92000000;
static u8 *BOOTER_ADDR = (u8*)0x92F00000;
static void (*entry)() = (void*)0x92F00000;
static struct __argv *ARGS_ADDR = (struct __argv*)0x93300800;

static GXRModeObj *rmode = NULL;

static void initGraphics()
{
	if(rmode)
		return;

	rmode = VIDEO_GetPreferredMode(NULL);
	if(!rmode)
		return;

	void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	if(!xfb)
		return;

	VIDEO_Init();
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
}

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

#ifdef DEBUG_BUILD
static inline void debugPrint(const char *msg, ...)
{
	initGraphics();
	printf(msg);
	sleep(2);
}
	#define nPrintf(x) debugPrint(x)
	#undef NO_DISPLAY
#else
	#define debugPrint(...)
static inline void nPrintf(const char *msg, ...)
{
	initGraphics();
	printf(msg);
	sleep(2);
}
#endif

static uint32_t getIdFromIso()
{
	if(WDVD_FST_OpenDisc(0) == 0)
	{
		uint32_t ret;
		if(WDVD_FST_Read((uint8_t *)&ret, 4) == 4)
			return ret;

		WDVD_FST_Close();
		debugPrint("Error reading iso!\n");
	}
	else
		debugPrint("Error opening iso!\n");

	return 0;
}

int main(int argc, char *argv[]) 
{
	debugPrint("Hello world! %s\n", "test");

	__io_wiisd.startup();
	__io_wiisd.isInserted();
	fatMount("sd", &__io_wiisd, 0, 4, 64);

	const char *fPath = "sd:/apps/nintendont/boot.dol";
	FILE *f = fopen(fPath,"rb");
	if(!f)
	{
		nPrintf("boot.dol not found!\n");
		sleep(2);
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
	ICInvalidateRange(BOOTER_ADDR,app_booter_bin_size);

	NIN_CFG nincfg;

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
		sleep(2);
		return -2;
	}

	char *fPath2 = "sd:/nintendont/configs/XXXX.bin";
	*(uint32_t *)(fPath2 + strlen("sd:/nintendont/configs/")) = fsize;
	f = fopen(fPath2,"rb");
	if(f)
	{
		if(fread(&nincfg,sizeof(NIN_CFG),1,f) == 1)
		{
			debugPrint("Game specific config loaded from SD!\n");
			goto clearNincfg;
		}

		debugPrint("Error reading game specific config from SD!\n");
		fclose(f);
	}
	else
		debugPrint("Error opening game specific config on SD!\n");

	fsize = 0;
	// TODO: Stupid workaround
	if(WDVD_FST_Unmount())
	{
		if(!WDVD_FST_Mount())
		{
			debugPrint("Error remounting iso!\n");
			fsize = 1;
		}
	}
	else
		debugPrint("Error unmounting iso!\n");

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

			// TODO: Remove / let injector set this correctly:
			memset(nincfg.GamePath,0,255);
			memset(nincfg.CheatPath,0,255);
			nincfg.Config |= (NIN_CFG_AUTO_BOOT);
			nincfg.Config &= ~(NIN_CFG_USB);
			strcpy(nincfg.GamePath,"di");
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
			WDVD_FST_Unmount();
			WDVD_Close();
			nPrintf("Error opening nincfg!\n");
			return -2;
		}

		if(fread(&nincfg,sizeof(NIN_CFG),1,f) != 1)
		{
			fclose(f);
			WDVD_FST_Unmount();
			WDVD_Close();
			nPrintf("Error reading nincfg!\n");
			return -2;
		}

		debugPrint("Fallback nincfg loaded!\n");

	clearNincfg:
		fclose(f);
		memset(nincfg.GamePath,0,255);
		memset(nincfg.CheatPath,0,255);
		nincfg.Config |= (NIN_CFG_AUTO_BOOT);
		nincfg.Config &= ~(NIN_CFG_USB);
		strcpy(nincfg.GamePath,"di");
	}

	WDVD_FST_Unmount();
	WDVD_Close();
	fatUnmount("sd:");
	__io_wiisd.shutdown();

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
	CMD_ADDR[fsize+sizeof(NIN_CFG)] = 0;
	DCFlushRange(ARGS_ADDR, full_args_len);

	deinitGraphics();

	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	__lwp_thread_stopmultitasking(entry);

	return 0;
}
