#ifndef KSUBSTR_H_
#define KSUBSTR_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


char *substr(char *src,int pos,int len);

char * mid(char *chaineSource, int pos);

char *right(char *chaineSource, int len);

char *left(char *chaineSource, int len);

int strpos(char *haystack, char *needle);

#endif
