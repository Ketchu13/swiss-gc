//simple string utils - ketchu13 2017
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOT_FOUND -1

char * substr(char *chaineSource,int pos,int len) {
// Retourne la sous-chaine de la chaine chaineSource
// depuis la position pos sur la longueur len
	char *dest=NULL;
	if(len>0) {
		/* allocation et mise à zéro */
		dest = (char *)calloc(len+1, 1);
		/* vérification de la réussite de l'allocation*/
		if(NULL != dest) {
			strncat(dest,chaineSource+pos,len);
		}
	}
	return dest;
}

char *mid(char *chaineSource, int pos) {
	// Retourne la sous-chaine de chaineSource depuis
	// la postion pos jusqu'au reste de chaineSource.
	// Si la position pos est supperieur à la longueur de chaineSource,
	// on retourne chaineSource.
	return (pos>strlen(chaineSource))? chaineSource : substr(chaineSource, pos, strlen(chaineSource));
}

char *left(char *chaineSource, int len){
	// Retourne les len caractères de gauche de chaineSource.
	// Si la longueur len est suppérieur ou égale à la longeur de chaineSource,
	// on retourne chaineSource. Si len est 0, on retourne '\0'.
	return (len>=strlen(chaineSource))? chaineSource : substr(chaineSource, 0, len);
}

char *right(char *chaineSource, int len){
	// Retourne les len caractères de droite de chaineSource.
	// Si la longueur len est suppérieur ou égale à la longueur de chaineSource,
	// on retourne chaineSource. Si len est 0, on retourne '\0'.
	return (len>=strlen(chaineSource))? chaineSource : substr(chaineSource, strlen(chaineSource)-len, len);
}

int strpos(char *haystack, char *needle){
	char *p = strstr(haystack, needle);
	if (p)
		return p - haystack;
	return NOT_FOUND;
}
