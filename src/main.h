#ifndef MAIN_H
#define MAIN_H

#define PLUGIN_VERSION "1.0"

#include <sdk/plugin.h>
#ifdef _WIN32
	#include <windows.h>
#endif
#include <string>

#define CHECK_PARAMS(m, n) \
	if (params[0] != (m * 4)) \
	{ \
		logprintf("*** %s: Expecting %d parameter(s), but found %d", n, m, params[0] / 4); \
		return 0; \
	}

class GPTRequest
{
public:
	GPTRequest(int playerid, std::string prompt)
	{
		this->playerid = playerid;
		this->prompt = prompt;
	}

	std::string GetPrompt()
	{
		return this->prompt;
	}	
	
	int GetPlayerID()
	{
		return this->playerid;
	}

private:
	std::string prompt;
	int playerid; //who requests
};

class GPTResponse
{
public:
	GPTResponse(int playerid, std::string response)
	{
		this->playerid = playerid;
		this->response = response;
	}

	std::string GetResponse()
	{
		return this->response;
	}

	int GetPlayerID()
	{
		return this->playerid;
	}

private:
	std::string response; //response of gpt
	int playerid; //who has requested
};


typedef void (*logprintf_t)(const char*, ...);

extern logprintf_t logprintf;
extern void *pAMXFunctions;

#endif
