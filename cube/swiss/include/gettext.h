/****************************************************************************
 * From Snes9x Nintendo Wii/Gamecube Port
 *
 * Tantric 2009-2010
 *
 ***************************************************************************/
// Adapted to the Greatest SwissGC of emu_kidid by ketchu13 2017

#ifndef _GETTEXT_H_
#define _GETTEXT_H_

bool LoadLanguage();

/*
 * input msg = a text in ASCII
 * output = the translated msg in utf-8
 */
char *gettext(char *msg);
const char *gettextc(const char *msgid);

#endif /* _GETTEXT_H_ */
