#ifndef WIN_COMPAT_H
#define WIN_COMPAT_H

#ifndef _WIN32
#include <cstring>

static inline void _co_noop(void*) {}
#define CoInitialize(x) _co_noop(x)
#define CoUninitialize()

#ifndef memcpy_s
#define memcpy_s(dest, destsz, src, count) memcpy(dest, src, count)
#endif

#endif /* _WIN32 */

#endif /* WIN_COMPAT_H */
