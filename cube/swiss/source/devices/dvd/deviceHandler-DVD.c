/* deviceHandler-DVD.c
	- device implementation for DVD (Discs)
	by emu_kidid
 */
//translation by ketchu13 15.42-19.3.17 windows-1252
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sys/dir.h>
#include <sys/statvfs.h>
#include "swiss.h"
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "main.h"
#include "patcher.h"
#include "dvd.h"
#include "gcm.h"
#include "wkf.h"
#include "frag.h"

#define OFFSET_NOTSET 0
#define OFFSET_SET    1

file_handle initial_DVD =
	{ "dvd:/",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
};

device_info initial_DVD_info = {
	TEX_GCDVDSMALL,
	1425760,
	1425760
};

static char error_str[256];
static int dvd_init = 0;
static int toc_read = 0;
char *dvdDiscTypeStr = NotInitStr;
int dvdDiscTypeInt = 0;
int drive_status = NORMAL_MODE;
int wkfDetected = 0;

char *dvd_error_str()
{
  unsigned int err = dvd_get_error();
  if(!err) return "OK";

  memset(&error_str[0],0,256);
  switch(err>>24) {
	case 0:
	  break;
	case 1:
	  strcpy(&error_str[0],"Couvercle ouvert");
	  break;
	case 2:
	  strcpy(&error_str[0],"No disk/Disk changed");
	  break;
	case 3:
	  strcpy(&error_str[0],"Pas de disk");
	  break;
	case 4:
	  strcpy(&error_str[0],"Moteur éteint");
	  break;
	case 5:
	  strcpy(&error_str[0],"Disque non initialisé");
	  break;
  }
  switch(err&0xFFFFFF) {
	case 0:
	  break;
	case 0x020400:
	  strcat(&error_str[0]," Moteur arrêté");
	  break;
	case 0x020401:
	  strcat(&error_str[0]," ID du disque non lu");
	  break;
	case 0x023A00:
	  strcat(&error_str[0]," Support non présent / couvercle ouvert");
	  break;
	case 0x030200:
	  strcat(&error_str[0]," Pas de recherche terminée");
	  break;
	case 0x031100:
	  strcat(&error_str[0]," Erreur de lecture non récupérée");
	  break;
	case 0x040800:
	  strcat(&error_str[0]," Erreur de protocole de transfert");
	  break;
	case 0x052000:
	  strcat(&error_str[0]," Code d'opération de commande non valide");
	  break;
	case 0x052001:
	  strcat(&error_str[0]," Buffer audio non défini");
	  break;
	case 0x052100:
	  strcat(&error_str[0]," Adresse de bloc logique hors limites");
	  break;
	case 0x052400:
	  strcat(&error_str[0]," Champ non valide dans le paquet de commande");
	  break;
	case 0x052401:
	  strcat(&error_str[0]," Commande audio non valide");
	  break;
	case 0x052402:
	  strcat(&error_str[0]," Configuration hors période autorisée");
	  break;
	case 0x053000:
	  strcat(&error_str[0]," DVD-R"); //?
	  break;
	case 0x053100:
	  strcat(&error_str[0]," Type de lecture incorrect"); //?
	  break;
	case 0x056300:
	  strcat(&error_str[0]," Fin de la zone utilisateur rencontrée sur cette piste");
	  break;
	case 0x062800:
	  strcat(&error_str[0]," Le support peut avoir changé");
	  break;
	case 0x0B5A01:
	  strcat(&error_str[0]," Demande de retrait du support d'opérateur");
	  break;
  }
  if(!error_str[0])
	sprintf(&error_str[0]," %08X Incconu",err);
  return &error_str[0];
}

int initialize_disc(u32 streaming) {
	int patched = NORMAL_MODE;
	DrawFrameStart();
	DrawProgressBar(33, "Initialisation du DVD..");
	DrawFrameFinish();
	if(is_gamecube())
	{
		// Reset WKF hard to allow for a real disc to be read if SD is removed
		if(wkfDetected || (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF)) {
			print_gecko("Wiikey Fusion détecté avec SPI Flash ID: %08X\r\n",__wkfSpiReadId());
			__wkfReset();
			print_gecko("WKF RESET\r\n");
			wkfDetected = 1;
		}

		DrawFrameStart();
		DrawProgressBar(40, "Réinitialisation du lecteur de DVD - Détection du support");
		DrawFrameFinish();
		dvd_reset();
		dvd_read_id();
		// Avoid lid open scenario
		if((dvd_get_error()>>24) && (dvd_get_error()>>24 != 1)) {
			DrawFrameStart();
			DrawProgressBar(75, "Possible sauvegarde de DVD - Activation des correctifs");
			DrawFrameFinish();
			dvd_enable_patches();
			if(!dvd_get_error()) {
				patched=DEBUG_MODE;
				drive_status = DEBUG_MODE;
			}
		}
		else if((dvd_get_error()>>24) == 1) {  // Lid is open, tell the user!
			DrawFrameStart();
			sprintf(txtbuffer, "Erreur %s. Appuyer sur A.",dvd_error_str());
			DrawMessageBox(D_FAIL, txtbuffer);
			DrawFrameFinish();
			wait_press_A();
			return DRV_ERROR;
		}
		if((streaming == ENABLE_AUDIO) || (streaming == DISABLE_AUDIO)) {
			dvd_set_streaming(streaming);
		}
		else {
			dvd_set_streaming(*(char*)0x80000008);
		}
		xeno_disable();
	}
	else {  //Wii, in GC Mode
		DVD_Reset(DVD_RESETHARD);
		dvd_read_id();
		if((streaming == ENABLE_AUDIO) || (streaming == DISABLE_AUDIO)) {
			dvd_set_streaming(streaming);
		}
		else {
			dvd_set_streaming(*(char*)0x80000008);
		}
	}
	dvd_read_id();
	if(dvd_get_error()) { //no disc, or no game id.
		DrawFrameStart();
		sprintf(txtbuffer, "Erreur: %s",dvd_error_str());
		DrawMessageBox(D_FAIL, txtbuffer);
		DrawFrameFinish();
		wait_press_A();
		dvd_reset();	// for good measure
		return DRV_ERROR;
	}
	DrawFrameStart();
	DrawProgressBar(100, "Initialisation Terminée");
	DrawFrameFinish();
	return patched;
}

int gettype_disc() {
  char *checkBuf = (char*)memalign(32,32);
  char *iso9660Buf = (char*)memalign(32,32);
  int type = UNKNOWN_DISC;
  dvdDiscTypeStr = UnkStr;
  
  // Don't assume there will be a valid disc ID at 0x80000000. 
  // Lets read the first 32b off the disc.
  DVD_Read((void*)checkBuf,0,32);
  DVD_Read((void*)iso9660Buf,32769,32);
  
  // Attempt to determine the disc type from the 32 byte header at 0x80000000
  if (strncmp(checkBuf, "COBRAM", 6) == 0) {
	dvdDiscTypeStr = CobraStr;
	type = COBRA_MULTIGAME_DISC;  //"COBRAM" is Cobra MultiBoot
  }
  else if ((strncmp(checkBuf, "GGCOSD", 6) == 0) || (strncmp(checkBuf, "RGCOSD", 6) == 0)) {
	dvdDiscTypeStr = GCOSD5Str;
	type = GCOSD5_MULTIGAME_DISC;  //"RGCOSD" or "GGCOSD" is GCOS Single Layer MultiGame Disc
  }
  else if (strncmp(checkBuf, "GCOPDVD9", 8) == 0) {
	dvdDiscTypeStr = GCOSD9Str;
	type = GCOSD9_MULTIGAME_DISC;  //"GCOPDVD9" is GCOS Dual Layer MultiGame Disc
  }
  else if (strncmp(iso9660Buf, "CD001", 5) == 0) {
	dvdDiscTypeStr = ISO9660Str;
	type = ISO9660_DISC;  //"CD001" at 32769 is iso9660
  }
  else if ((*(volatile unsigned int*)(&checkBuf[0x1C])) == DVD_MAGIC) {
	if(checkBuf[6]) {
	  dvdDiscTypeStr = MDStr;
	  type = MULTIDISC_DISC;  //Gamecube 2 Disc (or more?) Game
	}
	else {
	  dvdDiscTypeStr = GCDStr;
	  type = GAMECUBE_DISC;   //Gamecube Game
	}
  }

  free(iso9660Buf);
  free(checkBuf);  
  return type;
}

device_info* deviceHandler_DVD_info() {
	return &initial_DVD_info;
}

int deviceHandler_DVD_readDir(file_handle* ffile, file_handle** dir, unsigned int type){

	unsigned int i = 0, isGC = is_gamecube();
	unsigned int  *tmpTable = NULL;
	char *tmpName  = NULL;
	u64 tmpOffset = 0LL;
	u32 usedSpace = 0;

	int num_entries = 0, ret = 0, num = 0;
	
	if(dvd_get_error() || !dvd_init) { //if some error
		ret = initialize_disc(ENABLE_BYDISK);
		if(ret == DRV_ERROR) {    //try init
			return -1; //fail
		}
		dvd_init = 1;
		toc_read = 0;
	}

	dvdDiscTypeInt = gettype_disc();
	if(dvdDiscTypeInt == UNKNOWN_DISC) {
		return -1;
	}

	//GCOS or Cobra MultiGame DVD Disc
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {

		if(drive_status == NORMAL_MODE) {
			// This means we're using a drivechip, so our multigame command will be different.
			isXenoGC = 1;
		}
		//Read in the whole table of offsets
		tmpTable = (unsigned int*)memalign(32,MAX_MULTIGAME*4);
		tmpName = (char*)memalign(32,512);
		memset(tmpTable,0,MAX_MULTIGAME*4);
		memset(tmpName,0,512);
		DVD_Read(&tmpTable[0],MULTIGAME_TABLE_OFFSET,MAX_MULTIGAME*4);

		// count entries
		for(i = 0; i < MAX_MULTIGAME; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if((tmpOffset) && (tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				num_entries++;
			}
		}

		if(num_entries <= 0) { return num_entries; }

		// malloc the directory structure
		*dir = malloc( num_entries * sizeof(file_handle) );

		// parse entries
		for(i = 0; i < MAX_MULTIGAME; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if((tmpOffset) && (tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				DVD_Read(&tmpName[0],tmpOffset+32, 512);
				sprintf( (*dir)[num].name,"%s.gcm", &tmpName[0] );
				(*dir)[num].fileBase = tmpOffset;
				(*dir)[num].offset = 0;
				(*dir)[num].size   = DISC_SIZE;
				(*dir)[num].fileAttrib	 = IS_FILE;
				(*dir)[num].meta = 0;
				(*dir)[num].status = OFFSET_NOTSET;
				num++;
			}
			free(tmpTable);
			free(tmpName);
		}
	}
	else if((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)) {
		// TODO: BCA entry (dump from drive RAM on a GC, dump via BCA command on Wii)
		// Virtual entries for FST entries :D
		num_entries = read_fst(ffile, dir, !toc_read ? &usedSpace:NULL);
	}
	else if(dvdDiscTypeInt == ISO9660_DISC) {
		// Call the corresponding DVD function
		num_entries = dvd_read_directoryentries(ffile->fileBase,ffile->size);
		// If it was not successful, just return the error
		if(num_entries <= 0) return -1;
		// Convert the DVD "file" data to fileBrowser_files
		*dir = malloc( num_entries * sizeof(file_handle) );
		int i;
		for(i=0; i<num_entries; ++i){
			strcpy( (*dir)[i].name, DVDToc->file[i].name );
			(*dir)[i].fileBase = (uint64_t)(((uint64_t)DVDToc->file[i].sector)*2048);
			(*dir)[i].offset = 0;
			(*dir)[i].size   = DVDToc->file[i].size;
			(*dir)[i].fileAttrib	 = IS_FILE;
			if(DVDToc->file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].fileAttrib   = IS_DIR;
			if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
			(*dir)[i].meta = 0;
			usedSpace += (*dir)[i].size;
		}
		//kill the large TOC so we can have a lot more memory ingame (256k more)
		free(DVDToc);
		DVDToc = NULL;

		if(strlen((*dir)[0].name) == 0)
			strcpy( (*dir)[0].name, ".." );
	}
	usedSpace >>= 10;
	initial_DVD_info.freeSpaceInKB = initial_DVD_info.totalSpaceInKB - usedSpace;
	return num_entries;
}

int deviceHandler_DVD_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_DVD_readFile(file_handle* file, void* buffer, unsigned int length){
	//print_gecko("read: status:%08X dst:%08X ofs:%08X base:%08X len:%08X\r\n"
	//						,file->status,(u32)buffer,file->offset,(u32)((file->fileBase) & 0xFFFFFFFF),length);
	u64 actualOffset = file->fileBase+file->offset;

	if(file->status == OFFSET_SET) {
		actualOffset = file->offset;
	}
	int bytesread = DVD_Read(buffer,actualOffset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

int deviceHandler_DVD_setupFile(file_handle* file, file_handle* file2) {

	// Multi-Game disc audio streaming setup
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {
		deviceHandler_seekFile(file, 0, DEVICE_HANDLER_SEEK_SET);
		deviceHandler_readFile(file,(unsigned char*)0x80000000,32);
		char streaming = *(char*)0x80000008;
		if(streaming && !isXenoGC) {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Veuillez patienter, mise en place du streaming audio en cours.");
			DrawFrameFinish();
			dvd_motor_off();
			print_gecko("Définir extension %08X\r\n",dvd_get_error());
			dvd_setextension();
			print_gecko("Définir extension - terminé\r\nUnlock %08X\r\n",dvd_get_error());
			dvd_unlock();
			print_gecko("Unlock - terminé\r\nDéboguage Moteur allumé %08X\r\n",dvd_get_error());
			dvd_motor_on_extra();
			print_gecko("Déboguage Moteur allumé - terminé\r\nDéfinition du statut %08X\r\n",dvd_get_error());
			dvd_setstatus();
			print_gecko("Définition du statut - terminé %08X\r\n",dvd_get_error());
			dvd_read_id();
			print_gecko("ID lu %08X\r\n",dvd_get_error());
			dvd_set_streaming(streaming);
			
		}
		dvd_set_offset(file->fileBase);
		file->status = OFFSET_SET;
		print_gecko("Streaming %s %08X\r\n",streaming?"Activé":"Desactivé",dvd_get_error());
	}
	// Check if there are any fragments in our patch location for this game
	if(savePatchDevice>=0) {
		print_gecko("Enregistre le périphérique de Patch trouvé\r\n");
		// If there are 2 discs, we only allow 5 fragments per disc.
		int maxFrags = (VAR_FRAG_SIZE/12), i = 0;
		u32 *fragList = (u32*)VAR_FRAG_LIST;
		
		memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
		
		// Look for .patchX files, if we find some, open them and add them as fragments
		file_handle patchFile;
		int patches = 0;
		for(i = 0; i < maxFrags; i++) {
			u32 patchInfo[3];
			patchInfo[0] = 0; patchInfo[1] = 0; 
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%s:/swiss_patches/%i",(savePatchDevice ? "sdb":"sda"), i);

			struct stat fstat;
			if(stat(&patchFile.name[0],&fstat)) {
				break;
			}
			patchFile.fp = fopen(&patchFile.name[0], "rb");
			if(patchFile.fp) {
				fseek(patchFile.fp, fstat.st_size-12, SEEK_SET);
				
				if((fread(&patchInfo, 1, 12, patchFile.fp) == 12) && (patchInfo[2] == SWISS_MAGIC)) {
					get_frag_list(&patchFile.name[0]);
					print_gecko("%i fichiers patch trouvés ofs 0x%08X len 0x%08X base 0x%08X\r\n", 
									i, patchInfo[0], patchInfo[1], frag_list->frag[0].sector);
					fclose(patchFile.fp);
					fragList[patches*3] = patchInfo[0];
					fragList[(patches*3)+1] = patchInfo[1];
					fragList[(patches*3)+2] = frag_list->frag[0].sector;
					patches++;
				}
				else {
					break;
				}
			}
		}
		// Disk 1 base sector
		*(volatile unsigned int*)VAR_DISC_1_LBA = fragList[2];
		// Disk 2 base sector
		*(volatile unsigned int*)VAR_DISC_2_LBA = fragList[2];
		// Currently selected disk base sector
		*(volatile unsigned int*)VAR_CUR_DISC_LBA = fragList[2];
		// Copy the current speed
		*(volatile unsigned int*)VAR_EXI_BUS_SPD = 192;
		// Card Type
		*(volatile unsigned int*)VAR_SD_TYPE = sdgecko_getAddressingType(savePatchDevice);
		// Copy the actual freq
		*(volatile unsigned int*)VAR_EXI_FREQ = EXI_SPEED16MHZ;
		// Device slot (0 or 1) // This represents 0xCC0068xx in number of u32's so, slot A = 0xCC006800, B = 0xCC006814
		*(volatile unsigned int*)VAR_EXI_SLOT = savePatchDevice * 5;
	}
	return 1;
}

int deviceHandler_DVD_init(file_handle* file){
  file->status = initialize_disc(ENABLE_BYDISK);
  if(file->status == DRV_ERROR){
	  DrawFrameStart();
	  DrawMessageBox(D_FAIL,"Impossible de monter le DVD. Appuyer sur A");
	  DrawFrameFinish();
	  wait_press_A();
	  return file->status;
  }
  dvd_init=1;
  return file->status;
}

int deviceHandler_DVD_deinit(file_handle* file) {
	dvd_motor_off();
	return 0;
}

int deviceHandler_DVD_closeFile(file_handle* file){
	return 0;
}
