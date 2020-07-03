#include <time.h>
#include <stdio.h>

#ifdef DEBUG

char buf[20];
struct tm *sTm;
time_t now;
#define LOG(x)                                  \
now = time(0);                                  \
sTm = gmtime(&now);                             \
strftime(buf, sizeof(buf), "%H:%M:%S", sTm);    \
printf("%s %s\n", buf, x);                      \

#else 

#define LOG(x)  \
do {            \
}while(0);      \

#endif