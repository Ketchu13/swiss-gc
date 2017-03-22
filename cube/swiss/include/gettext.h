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
