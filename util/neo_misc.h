/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#ifndef __NEO_MISC_H_
#define __NEO_MISC_H_ 1

__BEGIN_DECLS

#include <stdarg.h>
#include <sys/types.h>

typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned short int UINT16;
typedef short int INT16;
typedef unsigned char UINT8;
typedef char INT8;
typedef char BOOL;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

void ne_vwarn (char *fmt, va_list ap);
void ne_warn (char *fmt, ...);
UINT32 python_string_hash (const char *s);
UINT8 *ne_stream4 (UINT8  *dest, UINT32 num);
UINT8 *ne_unstream4 (UINT32 *pnum, UINT8 *src);
UINT8 *ne_stream2 (UINT8  *dest, UINT16 num);
UINT8 *ne_unstream2 (UINT16 *pnum, UINT8 *src);
UINT8 *ne_stream_str (UINT8 *dest, char *s, int l);
UINT8 *ne_unstream_str (char *s, int l, UINT8 *src);
double ne_timef (void);
NEOERR *ne_mkdirs (char *path, mode_t mode);
NEOERR *ne_load_file (char *path, char **str);
UINT32 ne_crc (UINT8 *data, UINT32 bytes);

__END_DECLS

#endif /* __NEO_MISC_H_ */
