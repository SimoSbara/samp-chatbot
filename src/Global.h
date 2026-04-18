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