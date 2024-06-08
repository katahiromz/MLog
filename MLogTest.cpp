// MLogTest.cpp --- The tests for MLog
// License: MIT

//#define MLOG_USE_STRSAFE
//#define UNICODE
//#define _UNICODE
//#define MLOG_FILE_OUTPUT TEXT("%TEMP%\\log.txt")
//#define MLOG_CODEPAGE CP_UTF8
//#define MLOG_UTF16_OUTPUT
//#define MLOG_DEBUG_OUTPUT
#include "MLog.h"

int main(void)
{
    mlog_trace_a("mlog_trace_a: TEST\n");
    mlog_trace_w(L"mlog_trace_w: TEST\n");
    mlog_trace(TEXT("mlog_trace_t: TEST\n"));
#ifdef UNICODE
    mlog_trace(L"mlog_trace: TEST\n");
#else
    mlog_trace(u8"mlog_trace: TEST\n");
#endif

#ifndef __cplusplus
    mlog_free();
#endif
}
