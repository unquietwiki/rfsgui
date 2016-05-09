//MPA 5-1-2005
//64bit
//Cleaned up includes from other files & put them here.

//Newer includes (expect from SDK)
#include <iostream.h>
#include <basetsd.h>
#include <sys/types.h>
#include <windows.h>
#include <commctrl.h>
#include <winuser.h>
#include <errno.h>
//Older includes
#include <stdio.h>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>

#define SUPPORT_WINDOWS_XP_PARTITIONS

#include "gtools.h"

#ifdef _MSC_VER
#define FMT_QWORD "I64d"
#else
#define FMT_QWORD "qd"
#endif

//MPA, 11-9-2005: replaced with function
//#define PBytesInMBytes(BYTES) ((BYTES)/1048576)

