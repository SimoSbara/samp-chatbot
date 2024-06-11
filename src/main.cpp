#include "main.h"

#include <sdk/plugin.h>

#include <string>
#include <mutex>
#include <queue>
#include <set>

#include "curl/curl.h"
#include "json.hpp"

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "libcurl-d_imp.lib")
#else
#pragma comment(lib, "libcurl_imp.lib")
#endif

#endif

using json = nlohmann::json;

static std::queue<GPTRequest> requestes;
static std::queue<GPTResponse> responses;

static std::mutex requestLock;
static std::mutex responseLock;
static std::mutex keyLock;
static std::mutex sysPromptLock;

static std::string apiKey;
static std::string systemPrompt; //for better context answering

static std::thread requestsThread;
static std::atomic<bool> running;

std::set<AMX*> interfaces;

logprintf_t logprintf;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) 
{
	size_t totalSize = size * nmemb;
	response->append((char*)contents, totalSize);
	return totalSize;
}

static void DoRequest(std::string prompt, int playerid)
{
	std::string baseUrl = "https://api.openai.com/v1/chat/completions";
	std::string response;

	keyLock.lock();
	std::string curKey = apiKey;
	keyLock.unlock();

	sysPromptLock.lock();
	std::string curSysPrompt = systemPrompt;
	sysPromptLock.unlock();

	CURL* curl = curl_easy_init();

	if (curl) {
		json requestData;
		requestData["model"] = "gpt-3.5-turbo";
		requestData["messages"][0]["role"] = "system";
		requestData["messages"][0]["content"] = curSysPrompt;
		requestData["messages"][1]["role"] = "user";
		requestData["messages"][1]["content"] = prompt;
		requestData["temperature"] = 0;

		std::string requestDataStr = requestData.dump().c_str();

		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + curKey).c_str());
		curl_easy_setopt(curl, CURLOPT_URL, baseUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestDataStr.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestDataStr.length());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) 
		{
			logprintf("\n\nGPT Request Failed! Error: %s\n", curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	// return response;
	json jresponse = json::parse(response);

	json choices = jresponse.at("choices");
	json choices0 = choices.at(0);
	json message = choices0.at("message");

	response = message.value("content", "");

	if (response.empty())
	{
		GPTResponse gptRes(playerid, response);

		responseLock.lock();
		responses.push(gptRes);
		responseLock.unlock();
	}
}

static void RequestsThread(void* params)
{
	while (running)
	{
		requestLock.lock();
		GPTRequest& curRequest = requestes.front();
		requestes.pop();
		requestLock.unlock();

		DoRequest(curRequest.GetPrompt(), curRequest.GetPlayerID());
	}
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	logprintf("\n\nGPT API Plugin by SimoSbara loaded\n", PLUGIN_VERSION);

	running = true;
	requestsThread = std::thread(RequestsThread, nullptr);
	requestsThread.detach();

	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	running = false;

	logprintf("\n\nGPT API Plugin by SimoSbara unloaded\n", PLUGIN_VERSION);    
}

static cell AMX_NATIVE_CALL n_RequestToGPT(AMX* amx, cell* params)
{
	char* pRequest = NULL;

	CHECK_PARAMS(2, "RequestToGPT"); //playerid, request string
	amx_StrParam(amx, params[1], pRequest);

	std::string request(pRequest);
	int playerID = static_cast<int>(params[2]);

	if (!request.empty())
	{
		GPTRequest newRequest(playerID, request);

		requestLock.lock();
		requestes.push(newRequest);
		requestLock.unlock();

		return 0;
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetAPIKey(AMX* amx, cell* params)
{
	char* pKey = NULL;

	CHECK_PARAMS(1, "SetAPIKey"); //api key string
	amx_StrParam(amx, params[1], pKey);

	std::string key(pKey);

	if (!key.empty())
	{
		keyLock.lock();
		apiKey = key;
		keyLock.unlock();

		return 0;
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetSystemPrompt(AMX* amx, cell* params)
{
	char* pPrompt = NULL;

	CHECK_PARAMS(1, "SetSystemPrompt"); //system prompt string
	amx_StrParam(amx, params[1], pPrompt);

	std::string prompt(pPrompt);

	if (!prompt.empty())
	{
		sysPromptLock.lock();
		systemPrompt = prompt;
		sysPromptLock.unlock();

		return 0;
	}

	return 1;
}

AMX_NATIVE_INFO natives[] =
{
	{ "RequestToGPT", n_RequestToGPT },
	{ "SetAPIKey", n_SetAPIKey },
	{ "SetSystemPrompt", n_SetSystemPrompt },
	{ 0, 0 }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	interfaces.insert(amx);
	return amx_Register(amx, natives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	interfaces.erase(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	if (!responses.empty())
	{
		responseLock.lock();
		GPTResponse& response = responses.front();
		responses.pop();
		responseLock.unlock();

		for (std::set<AMX*>::iterator a = interfaces.begin(); a != interfaces.end(); a++)
		{
			cell amxAddresses[2] = { 0 };
			int amxIndex = 0;

			if (!amx_FindPublic(*a, "OnGPTResponse", &amxIndex))
			{
				//parametri al contrario
				amx_Push(*a, response.GetPlayerID());
				amx_PushString(*a, &amxAddresses[0], NULL, response.GetResponse().c_str(), 0, 0);
				amx_Exec(*a, NULL, amxIndex);
				amx_Release(*a, amxAddresses[0]);
			}
		}
	}
}