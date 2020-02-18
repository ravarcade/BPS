/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

namespace dbg {
	class CLog {
	public:
		static void Log(const char *format, va_list args) {
			auto len = vsnprintf(NULL, 0, format, args);
			char *buf = new char[len + 1];
			vsnprintf(buf, len, format, args);
			TRACE(buf);
			delete buf;
		}

		static void Log(const char *format, ...) {
			va_list args;
			va_start(args, format);
			Log(format, args);
			va_end(args);

		}

		static void Log(const wchar_t *format, va_list args) {
			auto len = vswprintf(NULL, 0, format, args);
			wchar_t *buf = new wchar_t[len + 1];
			vswprintf(buf, len, format, args);
			TRACEW(buf);
			delete buf;
		}

		static void Log(const wchar_t *format, ...) {
			va_list args;
			va_start(args, format);
			Log(format, args);
			va_end(args);
		}
	};

	inline void Log(const char *format, ...) {
		va_list args;
		va_start(args, format);
		CLog::Log(format, args);
		va_end(args);
	}

	inline void Log(const wchar_t *format, ...) {
		va_list args;
		va_start(args, format);
		CLog::Log(format, args);
		va_end(args);
	}
}