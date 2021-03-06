/*
*
*   Swiss - The Gamecube IPL replacement
*
*	frag.c
*		- Wrap normal read requests around a fragmented file table
*/

#include "../../reservedarea.h"
#include "common.h"

// Returns the amount read from the given offset until a frag is hit
u32 read_frag(void *dst, u32 len, u32 offset) {

	u32 *fragList = (u32*)VAR_FRAG_LIST;
	int isDisc2 = (*(u32*)(VAR_DISC_1_LBA)) != (*(u32*)VAR_CUR_DISC_LBA);
	int maxFrags = (*(u32*)(VAR_DISC_1_LBA) != *(u32*)(VAR_DISC_2_LBA)) ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0, j = 0;
	int fragTableStart = isDisc2 ? (maxFrags*3) : 0;
	int amountToRead = len;
	int adjustedOffset = offset;
	
	// Locate this offset in the fat table and read as much as we can from a single fragment
	for(i = 0; i < maxFrags; i++) {
		int fragTableIdx = fragTableStart +(i*3);
		int fragOffset = fragList[fragTableIdx+0];
		int fragSize = fragList[fragTableIdx+1];
		int fragSector = fragList[fragTableIdx+2];
		int fragOffsetEnd = fragOffset + fragSize;

		// Find where our read starts and read as much as we can in this frag before returning
		if(offset >= fragOffset && offset < fragOffsetEnd) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				amountToRead = fragOffsetEnd - offset;
			}
			if(fragOffset != 0) {
				adjustedOffset = offset - fragOffset;
			}
			do_read(dst, amountToRead, adjustedOffset, fragSector);
			return amountToRead;
		}
	}
	return 0;
}

void device_frag_read(void *dst, u32 len, u32 offset)
{
	void *oDst = dst;
	u32 oLen = len;
		
	while(len != 0) {
		int amountRead = read_frag(dst, len, offset);
		len-=amountRead;
		dst+=amountRead;
		offset+=amountRead;
	}
}

unsigned long tb_diff_usec(tb_t* end, tb_t* start)
{
	unsigned long upper, lower;
	upper = end->u - start->u;
	if (start->l > end->l)
		upper--;
	lower = end->l - start->l;
	return ((upper * ((unsigned long)0x80000000 / (TB_CLOCK / 2000000))) + (lower / (TB_CLOCK / 1000000)));
}

void calculate_speed(void* dst, u32 len, u32 *speed)
{
	tb_t start, end;
	mftb(&start);
	device_frag_read(dst, len, 0);
	mftb(&end);
	*speed = tb_diff_usec(&end, &start);
}
