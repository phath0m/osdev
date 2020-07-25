#ifndef _STRING_H
#define _STRING_H

#include <stdarg.h>
#include <sys/types.h>

int         atoi(const char *, int);

/* not actually part of the ansii string.h */
char    *   itoa(int, char *, int);
char    *   itoa_u(unsigned int, char *, int);

void    *   memcpy(void *, const void *, size_t);
void    *   memset(void *, int, size_t);

void        sprintf(char *, const char *, ...);
char    *   strchr(const char *, char);
char    *   strcpy(char *, const char *);
int         strlen(const char *);
int         strcmp(const char *, const char *);
char    *   strncpy(char *, const char *, size_t);
int         strncmp(const char *, const char *, size_t);
char    *   strrchr(const char *, char);
void        strreverse(char *);
void        vsprintf(char *, const char *, va_list);

#endif
