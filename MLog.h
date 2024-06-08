// MLog.h --- A simple logging library for C++/Win32 by katahiromz
// License: MIT
#pragma once

#define MLOG_VERSION 2

// Option: MLOG_MAX_BUF: 1024
// Option: MLOG_CODEPAGE: CP_UTF8
// Option: MLOG_USE_STRSAFE: (undefined)
// Option: MLOG_DEBUG_OUTPUT: (undefined)
// Option: MLOG_CONSOLE_OUTPUT: (undefined)
// Option: MLOG_FILE_OUTPUT: (undefined)
// Option: MLOG_UTF16_OUTPUT: (undefined)
// Option: MLOG_REGKEY: (undefined)

//////////////////////////////////////////////////////////////////////////////

#ifndef MLOG_MAX_BUF
    #define MLOG_MAX_BUF 1024
#endif

#ifndef MLOG_CODEPAGE
    #define MLOG_CODEPAGE CP_UTF8
#endif

#if !defined(MLOG_FILE_OUTPUT) && !defined(MLOG_CONSOLE_OUTPUT) && !defined(MLOG_DEBUG_OUTPUT)
    #define MLOG_CONSOLE_OUTPUT
#endif

//////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_TCHAR
    #include <tchar.h>
#endif

#ifdef __cplusplus
    #include <cstdio>
#else
    #include <stdio.h>
#endif

#ifdef MLOG_USE_STRSAFE
    #include <strsafe.h>
#endif

//////////////////////////////////////////////////////////////////////////////

C_ASSERT(MLOG_MAX_BUF >= MAX_PATH);

#ifdef MLOG_DEBUG_OUTPUT
    #undef MLOG_UTF16_OUTPUT
    #undef MLOG_CONSOLE_OUTPUT
#endif

#ifdef MLOG_CONSOLE_OUTPUT
    #undef MLOG_UTF16_OUTPUT
#endif

#if defined(UNICODE) != defined(_UNICODE)
    #error Macro UNICODE and _UNICODE are mismatching.
#endif

//////////////////////////////////////////////////////////////////////////////

typedef struct MLOG
{
    BOOL bInit;
    CRITICAL_SECTION csLock;
    TCHAR log_filename[MAX_PATH];
    BOOL bEnabled;

#ifdef __cplusplus
    MLOG()
    {
        bInit = FALSE;
        log_filename[0] = 0;
    }
    ~MLOG()
    {
        if (!bInit)
            return;
        DeleteCriticalSection(&csLock);
        bInit = FALSE;
    }
#endif
} MLOG, *PMLOG;

//////////////////////////////////////////////////////////////////////////////
// mlog_...

static inline
PMLOG mlog_init(void)
{
#ifdef __cplusplus
    static MLOG s_mlog;
#else
    static MLOG s_mlog = { FALSE };
#endif

    if (s_mlog.bInit)
        return &s_mlog;

    InitializeCriticalSection(&s_mlog.csLock);

#ifdef MLOG_DEBUG_OUTPUT
    ExpandEnvironmentStrings(TEXT(""), s_mlog.log_filename, _countof(s_mlog.log_filename));
#elif defined(MLOG_CONSOLE_OUTPUT)
    ExpandEnvironmentStrings(TEXT("con"), s_mlog.log_filename, _countof(s_mlog.log_filename));
#else
    ExpandEnvironmentStrings(MLOG_FILE_OUTPUT, s_mlog.log_filename, _countof(s_mlog.log_filename));
#endif

    s_mlog.bEnabled = TRUE;
    s_mlog.bInit = TRUE;
    return &s_mlog;
}

static inline
void mlog_free(void)
{
    PMLOG pmlog = mlog_init();
    if (!pmlog->bInit)
        return;
    DeleteCriticalSection(&pmlog->csLock);
    pmlog->bInit = FALSE;
}

static inline
void mlog_enable(BOOL bEnabled)
{
    PMLOG pmlog = mlog_init();
    if (pmlog)
        pmlog->bEnabled = bEnabled;
}

static inline
void mlog_lock(PMLOG pmlog)
{
    EnterCriticalSection(&pmlog->csLock);
}

static inline
void mlog_unlock(PMLOG pmlog)
{
    LeaveCriticalSection(&pmlog->csLock);
}

static inline
LPCTSTR mlog_log_file(void)
{
    PMLOG pmlog = mlog_init();
    if (!pmlog)
        return NULL;
    return pmlog->log_filename;
}

static inline
BOOL mlog_is_enabled(void)
{
    PMLOG pmlog = mlog_init();
    if (!pmlog)
        return FALSE;
    if (!pmlog->bEnabled)
        return FALSE;
#ifdef MLOG_REGKEY
    {
        HKEY hKey;
        DWORD bDisabled = FALSE;
        LONG error = RegOpenKeyEx(HKEY_CURRENT_USER, MLOG_REGKEY, 0, KEY_READ, &hKey);
        if (!error)
        {
            DWORD cbValue = sizeof(bDisabled);
            RegQueryValueEx(hKey, TEXT("DisableLogging"), NULL, NULL,
                            (BYTE*)&bDisabled, &cbValue);
            RegCloseKey(hKey);
        }
        if (bDisabled)
            return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// mlog_write...

static inline
void mlog_write_a(PMLOG pmlog, const char *ptr)
{
    FILE *fout;
#ifdef MLOG_UTF16_OUTPUT
    WCHAR buf[MLOG_MAX_BUF];
#endif

    if (!pmlog->bEnabled)
        return;

#ifdef MLOG_DEBUG_OUTPUT
    OutputDebugStringA(ptr);
    return;
#elif defined(MLOG_CONSOLE_OUTPUT)
    fout = stdout;
#elif defined(MLOG_UTF16_OUTPUT)
    fout = _tfopen(pmlog->log_filename, _T("ab"));
#else
    fout = _tfopen(pmlog->log_filename, _T("a"));
#endif

    if (!fout)
        return;

#ifdef MLOG_UTF16_OUTPUT
    MultiByteToWideChar(MLOG_CODEPAGE, 0, ptr, -1, buf, _countof(buf));
    buf[_countof(buf) - 1] = 0;
    fwrite(buf, lstrlenW(buf) * sizeof(WCHAR), 1, fout);
#else
    fputs(ptr, fout);
#endif

    if (fout != stdout)
        fclose(fout);
}

static inline
void mlog_write_w(PMLOG pmlog, const WCHAR *ptr)
{
    FILE *fout;
#ifndef MLOG_UTF16_OUTPUT
    CHAR buf[MLOG_MAX_BUF];
#endif

    if (!pmlog->bEnabled)
        return;

#ifdef MLOG_DEBUG_OUTPUT
    OutputDebugStringW(ptr);
    return;
#elif defined(MLOG_CONSOLE_OUTPUT)
    fout = stdout;
#elif defined(MLOG_UTF16_OUTPUT)
    fout = _tfopen(pmlog->log_filename, _T("ab"));
#else
    fout = _tfopen(pmlog->log_filename, _T("a"));
#endif

    if (!fout)
        return;

#ifndef MLOG_UTF16_OUTPUT
    WideCharToMultiByte(MLOG_CODEPAGE, 0, ptr, -1, buf, _countof(buf), NULL, NULL);
    buf[_countof(buf) - 1] = 0;
    fwrite(buf, lstrlenA(buf) * sizeof(CHAR), 1, fout);
#else
    fwrite(ptr, lstrlenW(ptr) * sizeof(WCHAR), 1, fout);
#endif

    if (fout != stdout)
        fclose(fout);
}

#ifdef UNICODE
    #define mlog_write mlog_write_w
#else
    #define mlog_write mlog_write_a
#endif

//////////////////////////////////////////////////////////////////////////////
// mlog_trace_ex...

static inline
void mlog_trace_ex_a(PMLOG pmlog, const char *file, int line, const char *fmt, ...)
{
    CHAR buf[MLOG_MAX_BUF];
    va_list va;
    int cch;
    va_start(va, fmt);
    mlog_lock(pmlog);
    if (pmlog->bEnabled)
    {
#ifdef MLOG_USE_STRSAFE
        StringCchPrintfA(buf, _countof(buf), "%s (%d): ", file, line);
        cch = lstrlenA(buf);
        StringCchVPrintfA(&buf[cch], _countof(buf) - cch, fmt, va);
#else
        cch = _snprintf(buf, _countof(buf), "%s (%d): ", file, line);
        _vsnprintf(&buf[cch], _countof(buf) - cch, fmt, va);
#endif
        mlog_write_a(pmlog, buf);
    }
    mlog_unlock(pmlog);
    va_end(va);
}

static inline
void mlog_trace_ex_w(PMLOG pmlog, const WCHAR *file, int line, const WCHAR *fmt, ...)
{
    WCHAR buf[MLOG_MAX_BUF];
    va_list va;
    int cch;
    va_start(va, fmt);
    mlog_lock(pmlog);
    if (pmlog->bEnabled)
    {
#ifdef MLOG_USE_STRSAFE
        StringCchPrintfW(buf, _countof(buf), L"%s (%d): ", file, line);
        cch = lstrlenW(buf);
        StringCchVPrintfW(&buf[cch], _countof(buf) - cch, fmt, va);
#else
        cch = _snwprintf(buf, _countof(buf), L"%s (%d): ", file, line);
        _vsnwprintf(&buf[cch], _countof(buf) - cch, fmt, va);
#endif
        mlog_write_w(pmlog, buf);
    }
    mlog_unlock(pmlog);
    va_end(va);
}

#ifdef UNICODE
    #define mlog_trace_ex mlog_trace_ex_w
#else
    #define mlog_trace_ex mlog_trace_ex_a
#endif

//////////////////////////////////////////////////////////////////////////////
// mlog_trace...

#define MLOG_WIDE(str) L##str
#define MLOG_WIDE_(str) MLOG_WIDE(str)

#define mlog_trace_a(fmt, ...) \
    mlog_trace_ex_a(mlog_init(), __FILE__ , __LINE__, fmt, ## __VA_ARGS__)

#define mlog_trace_w(fmt, ...) \
    mlog_trace_ex_w(mlog_init(), MLOG_WIDE_(__FILE__), __LINE__, fmt, ## __VA_ARGS__)

#ifdef UNICODE
    #define mlog_trace mlog_trace_w
#else
    #define mlog_trace mlog_trace_a
#endif

//////////////////////////////////////////////////////////////////////////////
