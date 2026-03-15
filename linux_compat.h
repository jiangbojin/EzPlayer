#ifndef LINUX_COMPAT_H
#define LINUX_COMPAT_H

#ifndef _WIN32
#include <string.h>
#define CoInitialize(x) ((void)0)
#define memcpy_s(dest, dsize, src, count) memcpy(dest, src, count)
#endif

#endif
