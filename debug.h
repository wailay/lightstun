#include <time.h>
#include <stdio.h>

#ifdef DEBUG

char time_buf[20];
struct tm *sTm;
time_t now;
#define LOG(fmt, ...)                                          \
now = time(0);                                          \
sTm = gmtime(&now);                                     \
strftime(time_buf, sizeof(time_buf), "%H:%M:%S", sTm);  \
printf("%s : ", time_buf);                                 \
printf(fmt, __VA_ARGS__);                                \

#else 

#define LOG(fmt,...)              \
fprintf(stderr, fmt, __VA_ARGS__); \

#endif