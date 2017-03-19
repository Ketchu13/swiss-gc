/**
*
* Gecko OS/WiiRD cheat engine (kenobigc)
* 
* Adapted to Swiss by emu_kidid 2012-2015
*
*/
//translation by ketchu13 13.16-19.3.17 windows-1252
#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>
#include "swiss.h"
#include "main.h"
#include "cheats.h"
#include "patcher.h"
#include "deviceHandler.h"
#include "FrameBufferMagic.h"

static CheatEntries _cheats;

void printCheats(void) {
	int i = 0, j = 0;
	print_gecko("Nombre de cheats: %i\r\n", _cheats.num_cheats);
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		print_gecko("Cheat: (%i codes) %s\r\n", cheat->num_codes, cheat->name);
		for(j = 0; j < cheat->num_codes; j++) {
			print_gecko("%08X %08X\r\n", cheat->codes[j][0], cheat->codes[j][1]);
		}
	}
}

int getEnabledCheatsSize(void) {
	int i = 0, size = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		if(cheat->enabled) {
			size += ((cheat->num_codes*2)*4);
		}
	}
	//print_gecko("Size of cheats: %i\r\n", size);
	return size;
}

static char *validCodeValues = "0123456789AaBbCcDdEeFf";
// Checks that the line contains a valid code in the format of "01234567 89ABCDEF"
int isValidCode(char *code) {
	int i, j;
	
	if(strlen(code) < 17) return 0; // Not in format
	for(i = 0; i < 16; i++) {
		if(i == 8 && code[i] != ' ') 
			return 0; // No space separating the two values
		if(i == 8)
			continue;
		int found = 0;
		for(j = 0; j < strlen(validCodeValues); j++) {
			if(code[i] == validCodeValues[j]) {
				found = 1;
				break;
			}
		}
		if(!found) return 0; 	// Wasn't a valid hexadecimal value
	}

	return 1;
}

int containsXX(char *line) {
	return (strlen(line)>=16 && (tolower((int)line[15]) == 'x'||tolower((int)line[15]) == 'y'));
}

/** 
	Given a char array with the contents of a .txt, 
	this method will allocate and return a populated Parameters struct 
*/
void parseCheats(char *filecontents) {
	char *line = NULL, *prevLine = NULL, *linectx = NULL;
	int numCheats = 0;
	line = strtok_r( filecontents, "\n", &linectx );

	memset(&_cheats, 0, sizeof(CheatEntries));
	
	CheatEntry *curCheat = NULL;	// The current one we're parsing
	while( line != NULL ) {
		//print_gecko("Line [%s]\r\n", line);
		
		if(isValidCode(line) && (prevLine != NULL && !containsXX(prevLine))) {		// The line looks like a valid code
			if(prevLine != NULL) {
				curCheat = &_cheats.cheat[numCheats];
				strncpy(curCheat->name, prevLine, strlen(prevLine) > CHEATS_NAME_LEN ? CHEATS_NAME_LEN-1:strlen(prevLine));
				if(curCheat->name[strlen(curCheat->name)-1] == '\r')
					curCheat->name[strlen(curCheat->name)-1] = 0;
				//print_gecko("Cheat Name: [%s]\r\n", prevLine);
			}
			int numCodes = 0, unsupported = 0;
			// Add this valid code as the first code for this cheat
			curCheat->codes[numCodes][0] = (u32)strtoul(line, NULL, 16);
			curCheat->codes[numCodes][1] = (u32)strtoul(line+8, NULL, 16);
			numCodes++;
			
			line = strtok_r( NULL, "\n", &linectx);
			// Keep going until we're out of codes for this cheat
			while( line != NULL ) {
				// If a code contains "XX" in it, it is unsupported, discard it entirely
				if(containsXX(line)) {
					unsupported = 1;
					break;
				}
				if(isValidCode(line)) {
					// Add this valid code
					curCheat->codes[numCodes][0] = (u32)strtoul(line, NULL, 16);
					curCheat->codes[numCodes][1] = (u32)strtoul(line+8, NULL, 16);
					numCodes++;
				}
				else {
					break;
				}
				line = strtok_r( NULL, "\n", &linectx);
			}
			curCheat->num_codes = numCodes;
			if(unsupported) {
				memset(curCheat, 0, sizeof(CheatEntry));
				while(line != NULL && strlen(line) > 0) {
					line = strtok_r( NULL, "\n", &linectx);	// finish this unsupported cheat.
				}
			}
			else 
				numCheats++;
		}
		prevLine = line;
		// And round we go again
		line = strtok_r( NULL, "\n", &linectx);
	}
	_cheats.num_cheats = numCheats;
	//printCheats();
}

CheatEntries* getCheats() {
	return &_cheats;
}

// Installs the GeckoOS (kenobiGC) cheats engine and sets up variables/copies cheats
void kenobi_install_engine() {
	int isDebug = swissSettings.wiirdDebug;
	// If high memory is in use, we'll use low, otherwise high.
	u8 *ptr = isDebug ? kenobigc_dbg_bin : kenobigc_bin;
	u32 size = isDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size;
	
	print_gecko("Copie de kenobi%s to %08X en cours\r\n", (isDebug?"_dbg":""),(u32)CHEATS_ENGINE);
	memcpy(CHEATS_ENGINE, ptr, size);
	memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);
	if(!isDebug) {
		CHEATS_ENABLE_CHEATS = CHEATS_TRUE;
	}
	CHEATS_START_PAUSED = isDebug ? CHEATS_TRUE : CHEATS_FALSE;
	memset(CHEATS_LOCATION(size), 0, kenobi_get_maxsize());
	print_gecko("Copie de %i octets de cheats sur %08X\r\n", getEnabledCheatsSize(),(u32)CHEATS_LOCATION(size));
	u32 *cheatsLocation = (u32*)CHEATS_LOCATION(size);
	cheatsLocation[0] = 0x00D0C0DE;
	cheatsLocation[1] = 0x00D0C0DE;
	cheatsLocation+=2;
	
	int i = 0, j = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		if(cheat->enabled) {
			for(j = 0; j < cheat->num_codes; j++) {
				// Copy & fix cheats that want to jump to the old cheat engine location 0x800018A8 -> CHEATS_ENGINE+0xA8
				cheatsLocation[0] = cheat->codes[j][0];
				cheatsLocation[1] = cheat->codes[j][1] == 0x800018A8 ? (u32)(CHEATS_ENGINE+0xA8) : cheat->codes[j][1];
				cheatsLocation+=2;
			}
		}
	}
	cheatsLocation[0] = 0xFF000000;
	DCFlushRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
	ICInvalidateRange((void*)CHEATS_ENGINE, WIIRD_ENGINE_SPACE);
}

int kenobi_get_maxsize() {
	return CHEATS_MAX_SIZE((swissSettings.wiirdDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size));
}

int findCheats(bool silent) {
	char trimmedGameId[8];
	memset(trimmedGameId, 0, 8);
	memcpy(trimmedGameId, (char*)&GCMDisk, 6);
	file_handle *cheatsFile = memalign(32,sizeof(file_handle));
	memcpy(cheatsFile, deviceHandler_initial, sizeof(file_handle));
	sprintf(cheatsFile->name, "%s/cheats/%s.txt", deviceHandler_initial->name, trimmedGameId);
	print_gecko("Recherche de fichier cheats @ %s\r\n", cheatsFile->name);
	cheatsFile->size = -1;
	
	deviceHandler_temp_readFile =  deviceHandler_readFile;
	deviceHandler_temp_seekFile =  deviceHandler_seekFile;
	
	// Check SD in both slots if we're not already running from SD, or if we fail from SD
	if((curDevice != SD_CARD && curDevice != IDEEXI && curDevice != WKF) || deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) != 8) {
		if(deviceHandler_initial != &initial_SD0 && deviceHandler_initial != &initial_SD1) {
			int slot = 0;
			deviceHandler_temp_init     =  deviceHandler_FAT_init;
			deviceHandler_temp_readFile =  deviceHandler_FAT_readFile;
			deviceHandler_temp_seekFile =  deviceHandler_FAT_seekFile;
			deviceHandler_temp_deinit   =  deviceHandler_FAT_deinit;
			while(slot < 2) {
				file_handle *slotFile = slot ? &initial_SD1:&initial_SD0;
				// Try SD slots now
				memcpy(cheatsFile, slotFile, sizeof(file_handle));
				sprintf(cheatsFile->name, "%s/cheats/%s.txt", slotFile->name, trimmedGameId);
				print_gecko("Recherche de fichier cheats @ %s\r\n", cheatsFile->name);
				cheatsFile->size = -1;
				deviceHandler_temp_init(cheatsFile);
				if(deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) == 8) {
					break;
				}
				slot++;
			}
		}
		// Still fail?
		if(deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) != 8) {
			if(!silent) {
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y);
				DrawFrameStart();
				DrawMessageBox(D_INFO,"Pas de fichier cheats trouv�.\nAppuyer sur A pour continuer.");
				DrawFrameFinish();
				while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
			}
			free(cheatsFile);
			return 0;
		}
	}
	print_gecko("Fichier cheat trouv� avec pour taille %i\r\n", cheatsFile->size);
	char *cheats_buffer = memalign(32, cheatsFile->size);
	if(cheats_buffer) {
		deviceHandler_temp_seekFile(cheatsFile, 0, DEVICE_HANDLER_SEEK_SET);
		deviceHandler_temp_readFile(cheatsFile, cheats_buffer, cheatsFile->size);
		parseCheats(cheats_buffer);
		free(cheats_buffer);
		free(cheatsFile);
	}
	return _cheats.num_cheats;
}

int applyAllCheats() {
	int i = 0, j = 0;
	for(i = 0; i < _cheats.num_cheats; i++) {
		CheatEntry *cheat = &_cheats.cheat[i];
		cheat->enabled = 1;
		if(getEnabledCheatsSize() > kenobi_get_maxsize())
			cheat->enabled = 0;
		j++;
	}
	return j;
}
