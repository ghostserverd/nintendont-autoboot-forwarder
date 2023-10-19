#include <gctypes.h>
#include "string.h"
#include "dolloader.h"
#include "sync.h"

#define ARENA1_HI_LIMIT 0x81800000

typedef struct _dolheader {
	u32 text_pos[7];
	u32 data_pos[11];
	void *text_start[7];
	void *data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	void *bss_start;
	u32 bss_size;
	entrypoint entry_point;
} dolheader;

entrypoint load_dol_image(const void *dolstart)
{
	dolheader *dolfile = (dolheader *) dolstart;

/* 	if (dolfile->bss_start > 0 && dolfile->bss_start < ARENA1_HI_LIMIT) {
		u32 bss_size = dolfile->bss_size;
		if (dolfile->bss_start + bss_size > ARENA1_HI_LIMIT) {
			bss_size = ARENA1_HI_LIMIT - dolfile->bss_start;
		}
		memset(dolfile->bss_start, 0, bss_size);
		sync_before_exec(dolfile->bss_start, bss_size);
	} */

	for(u32 i = 0; i < 11; i++)
	{
		if(i < 7 && dolfile->text_size[i])
		{
			memcpy(dolfile->text_start[i], dolstart + dolfile->text_pos[i], dolfile->text_size[i]);
			sync_before_exec(dolfile->text_start[i], dolfile->text_size[i]);
		}

		if(dolfile->data_size[i])
		{
			memcpy(dolfile->data_start[i], dolstart + dolfile->data_pos[i], dolfile->data_size[i]);
			sync_before_exec(dolfile->data_start[i], dolfile->data_size[i]);
		}
	}

	return dolfile->entry_point;
}
