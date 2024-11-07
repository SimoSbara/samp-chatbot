#include "main.h"

#include <sdk/plugin.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <set>

#include "curl/curl.h"
#include "json.hpp"

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "libcurl_debug.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif

#endif

char EncodingHelper::accentFilters[65536]; //look up table
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> EncodingHelper::converter;

static std::queue<AIRequest> requestes;
static std::queue<AIResponse> responses;

static std::mutex requestLock;
static std::mutex responseLock;
static std::mutex keyLock;
static std::mutex sysPromptLock;
static std::mutex modelLock;
static std::mutex chatBotLock;

static std::string apiKey;
static std::string systemPrompt; //for better context answering
static std::string model = "gpt-3.5-turbo";
static unsigned char chatBotType = GPT; //0 - GPT, 1 - Gemini, 2 - LLAMA (https://groq.com/)

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

static curl_slist* GetHeader(int type, std::string curAPIKey)
{
	curl_slist* headers = NULL;

	switch (type)
	{
	case GPT:
	case LLAMA:
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + curAPIKey).c_str());
		break;
	case GEMINI:
		headers = curl_slist_append(headers, "Content-Type: application/json");
		break;
	}

	return headers;
}

static std::string GetURL(int type, std::string curAPIKey, std::string curModel)
{
	switch (type)
	{
	case GPT:
		return "https://api.openai.com/v1/chat/completions";
	case GEMINI:
		return "https://generativelanguage.googleapis.com/v1beta/models/" + curModel + ":generateContent?key=" + curAPIKey;
	case LLAMA:
		return "https://api.groq.com/openai/v1/chat/completions";
	}

	return "";
}

static nlohmann::json CreateRequest(int type, std::string prompt, std::string curSysPrompt, std::string curModel)
{
	nlohmann::json requestDoc;

	switch (type)
	{
	case LLAMA:
	case GPT:
	{
		int index = 0;

		requestDoc["model"] = curModel;
		if (!curSysPrompt.empty())
		{
			requestDoc["messages"][index]["role"] = "system";
			requestDoc["messages"][index]["content"] = curSysPrompt;
			index++;
		}
		requestDoc["messages"][index]["role"] = "user";
		requestDoc["messages"][index]["content"] = prompt;
		requestDoc["temperature"] = 0;
	}
		break;
	case GEMINI:
	{
		if(!curSysPrompt.empty())
			requestDoc["system_instruction"]["parts"]["text"] = curSysPrompt;
		requestDoc["contents"][0]["parts"][0]["text"] = prompt;
	}
		break;
	}

	return requestDoc;
}

static std::string GetBotAnswer(int type, nlohmann::json response)
{
	if (!response.empty())
	{
		try
		{
			std::string answer;

			try
			{
				nlohmann::json error = response.at("error");

				if (!error.empty())
					return response.dump(4).c_str();
			}
			catch (std::exception)
			{
				//errore non trovato
			}

			switch (type)
			{
			case LLAMA:
			case GPT:
				answer = response.at("choices").at(0).at("message").at("content");
				break;
			case GEMINI:
				answer = response.at("candidates").at(0).at("content").at("parts").at(0).at("text");
				break;
			}

			return EncodingHelper::FilterAccents(answer);
		}
		catch (std::exception exc)
		{
			logprintf("ChatBot Plugin Exception GetBotAnswer(): %s\n", exc.what());
			logprintf("Chatbot Plugin Exception Response:\n%s", response.dump(4).c_str());

			return response.dump(4).c_str();
		}
	}

	return "";
}

static void DoRequest(std::string prompt, int id)
{
	std::string response;

	keyLock.lock();
	std::string curKey = apiKey;
	keyLock.unlock();

	sysPromptLock.lock();
	std::string curSysPrompt = systemPrompt;
	sysPromptLock.unlock();

	modelLock.lock();
	std::string curModel = model;
	modelLock.unlock();

	chatBotLock.lock();
	int curChatBot = chatBotType;
	chatBotLock.unlock();

	CURL* curl = curl_easy_init();

	if (curl) 
	{
		std::string url = GetURL(curChatBot, curKey, curModel);
		nlohmann::json requestData = CreateRequest(curChatBot, prompt, curSysPrompt, curModel);
		curl_slist* headers = GetHeader(curChatBot, curKey);

		std::string requestDataStr = requestData.dump().c_str();

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestDataStr.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestDataStr.length());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) 
		{
			logprintf("\n\nChat Bot Request Failed! Error: %s\n", curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	nlohmann::json jresponse = nlohmann::json::parse(response);

	std::string answer = GetBotAnswer(curChatBot, jresponse);

	AIResponse resp(id, prompt, answer);

	responseLock.lock();
	responses.push(resp);
	responseLock.unlock();
}

static void RequestsThread()
{
	using namespace std::chrono_literals;
	AIRequest curRequest;

	while (running)
	{
		requestLock.lock();

		if (!requestes.empty())
		{
			curRequest = requestes.front();
			requestes.pop();
		}

		requestLock.unlock();

		std::string prompt = curRequest.GetPrompt();
		int id = curRequest.GetID();

		if (!prompt.empty())
		{
#ifdef _DEBUG
			logprintf("\nnew request: %s\n", prompt.c_str());
#endif
			DoRequest(prompt, id);
			curRequest.Clear();	
		}
		else
		    std::this_thread::sleep_for(10ms);

		curRequest.Clear();
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
	logprintf("\n\nChatBot API Plugin %s by SimoSbara loaded\n", PLUGIN_VERSION);

	EncodingHelper::Init();

	running = true;
	requestsThread = std::thread(RequestsThread);
	requestsThread.detach();

	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	running = false;

	logprintf("\n\nChatBot API Plugin %s by SimoSbara unloaded\n", PLUGIN_VERSION);    
}

static cell AMX_NATIVE_CALL n_RequestToChatBot(AMX* amx, cell* params)
{
	char* pRequest = NULL;

	CHECK_PARAMS(2, "RequestToChatBot"); //id int, request string

	amx_StrParam(amx, params[1], pRequest);

	int id = static_cast<int>(params[2]);

	if (pRequest)
	{
		AIRequest newRequest(id, std::string(pRequest));

		requestLock.lock();
		requestes.push(newRequest);
		requestLock.unlock();

		return 1;
	}

	return 0;
}

static cell AMX_NATIVE_CALL n_SelectChatBot(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "SelectChatBot"); //api key string

	int type = static_cast<int>(params[1]);

	if (type >= GPT && type < NUM_CHAT_BOTS)
	{
		chatBotLock.lock();
		chatBotType = type;
		chatBotLock.unlock();

		return 1;
	}

	return 0;
}

static cell AMX_NATIVE_CALL n_SetAPIKey(AMX* amx, cell* params)
{
	char* pKey = NULL;

	CHECK_PARAMS(1, "SetAPIKey"); //api key string
	amx_StrParam(amx, params[1], pKey);

	if (pKey)
	{
		keyLock.lock();
		apiKey = std::string(pKey);
		keyLock.unlock();

		return 1;
	}

	return 0;
}

static cell AMX_NATIVE_CALL n_SetSystemPrompt(AMX* amx, cell* params)
{
	char* pPrompt = NULL;

	CHECK_PARAMS(1, "SetSystemPrompt"); //system prompt string
	amx_StrParam(amx, params[1], pPrompt);

	sysPromptLock.lock();

	if (pPrompt)
		systemPrompt = std::string(pPrompt);
	else
		systemPrompt.clear();

	sysPromptLock.unlock();

	return 1;
}

static cell AMX_NATIVE_CALL n_SetModel(AMX* amx, cell* params)
{
	char* pModel = NULL;

	CHECK_PARAMS(1, "SetModel"); //chatbot model string
	amx_StrParam(amx, params[1], pModel);

	if (pModel)
	{
		modelLock.lock();
		model = std::string(pModel);
		modelLock.unlock();

		return 1;
	}

	return 0;
}

AMX_NATIVE_INFO natives[] =
{
	{ "RequestToChatBot", n_RequestToChatBot },
	{ "SelectChatBot", n_SelectChatBot },
	{ "SetAPIKey", n_SetAPIKey },
	{ "SetSystemPrompt", n_SetSystemPrompt },
	{ "SetModel", n_SetModel },
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
		AIResponse response = responses.front();
		responses.pop();
		responseLock.unlock();

#ifdef _DEBUG
		logprintf("\nnew response: %s\n", response.GetResponse().c_str());
#endif

		for (std::set<AMX*>::iterator a = interfaces.begin(); a != interfaces.end(); a++)
		{
			cell amxAddresses[2] = { 0 };
			int amxIndex = 0;

			if (!amx_FindPublic(*a, "OnChatBotResponse", &amxIndex))
			{
				//parametri al contrario
				amx_Push(*a, response.GetID());
				amx_PushString(*a, &amxAddresses[0], NULL, response.GetResponse().c_str(), 0, 0);
				amx_PushString(*a, &amxAddresses[1], NULL, response.GetPrompt().c_str(), 0, 0);
				amx_Exec(*a, NULL, amxIndex);
				amx_Release(*a, amxAddresses[0]);
				amx_Release(*a, amxAddresses[1]);
			}
		}
	}
}
