#include "platform.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#endif

#ifdef _MSC_VER
#include <crtdbg.h>
extern "C" int __stdcall IsDebuggerPresent(void);
struct check_for_leaks_at_exit {
    ~check_for_leaks_at_exit() {
        // Only report leaks if destructors run (so they only show up on clean exits)
        _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
    }
} check_for_leaks_at_exit_;
#endif

namespace mjs {

void platform_init() {

#ifdef _MSC_VER
    (void)check_for_leaks_at_exit_;
    if (!IsDebuggerPresent()) {
        _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    }
#endif

#ifdef _WIN32
    if (_isatty(_fileno(stdin))) {
        _setmode(_fileno(stdin), _O_U16TEXT);
    }
    if (_isatty(_fileno(stdout))) {
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);
    }
#endif

}

} // namespace mjs
