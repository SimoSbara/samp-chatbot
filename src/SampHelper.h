#pragma once

#define PLUGIN_VERSION "v1.5"

#include <sdk/plugin.h>
#include <set>

#ifdef _WIN32
	#include <windows.h>
#endif

typedef void (*logprintf_t)(const char*, ...);

#define CHECK_PARAMS(m, n) \
	if (params[0] != (m * 4)) \
	{ \
		logprintf("*** %s: Expecting %d parameter(s), but found %d", n, m, params[0] / 4); \
		return 0; \
	}

static logprintf_t logprintf;

extern void* pAMXFunctions;
static std::set<AMX*> interfaces;