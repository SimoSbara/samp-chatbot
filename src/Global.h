#pragma once

#include <atomic>
#include "EncodingHelper.hpp"

enum LogModes
{
	LOG_SILENT = 0,
	LOG_ERRORS,
	LOG_VERBOSE,
	NUM_LOG_MODES
};

struct GlobalParams
{
	std::atomic<Encodings> encoding = W1252; //windows 1252
	std::atomic<uint16_t> timeout = 0; //milliseconds
	std::atomic<LogModes> logMode = LOG_ERRORS; //errors only
};

extern GlobalParams globalParams;
extern logprintf_t logprintf;

static void WriteInfoLog(std::string info)
{
	if (!logprintf)
		return;

	if (globalParams.logMode != LOG_SILENT)
		logprintf("[ChatBot Plugin] Info: %s\n", info.c_str());
}

static void WriteErrorLog(std::string error)
{
	if (!logprintf)
		return;

	if (globalParams.logMode >= LOG_ERRORS)
		logprintf("[ChatBot Plugin] Error: %s\n", error.c_str());
}

static void WriteVerboseLog(std::string info)
{
	if (!logprintf)
		return;

	if (globalParams.logMode >= LOG_VERBOSE)
		logprintf("[ChatBot Plugin] Verbose Info: %s\n", info.c_str());
}