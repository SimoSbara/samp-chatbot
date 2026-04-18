#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <condition_variable>

#include "Global.h"
#include "SampHelper.h"
#include "EncodingHelper.hpp"
#include "ChatBotHelper.hpp"
#include "ChatBot.hpp"

static std::unordered_map<int, ChatBot> chatBots;
static std::unordered_map<int, ChatMemory> chatMemories;

static int requestIncrementalID = 0;
static std::queue<AIRequest> requestes;
static std::queue<AIResponse> responses;

static std::mutex chatBotLock;
static std::mutex requestLock;
static std::mutex responseLock;
static std::mutex memoryLock;

static std::condition_variable cvRequest;
static std::thread requestsThread;
static std::atomic<bool> running;

GlobalParams globalParams;

logprintf_t logprintf;

static void AppendResponse(const AIResponse& response)
{
	std::lock_guard<std::mutex> lk(responseLock);

	responses.push(response);
}

static void ExecuteRequest(const AIRequest& request)
{
	std::string answer;

	int requestID = request.GetID();
	int chatbotID = request.GetChatBotID();
	int memoryID = request.GetMemoryID();
	std::string prompt = request.GetPrompt();

	ChatMemory memory;
	ChatBot chatbot;

	AIResponse response(requestID, prompt, "");
	StatusCodes status = SUCCESS;

	//lo trovo?
	if (memoryID != -1)
	{
		std::lock_guard<std::mutex> lk(memoryLock);

		if (chatMemories.find(memoryID) != chatMemories.end())
			memory = chatMemories[memoryID];
		else
			status = INVALID_MEMORY;
	}

	if (status != SUCCESS)
	{
		AppendResponse(response);
		return;
	}

	{
		std::lock_guard<std::mutex> lk(chatBotLock);

		if (chatBots.find(memoryID) != chatBots.end())
			chatbot = chatBots[memoryID];
		else
			status = INVALID_CHATBOT;
	}

	if (status != SUCCESS)
	{
		AppendResponse(response);
		return;
	}

	if (!ChatBotHelper::DoRequest(answer, prompt, chatbot, memory))
		status = FAILED_REQUEST;
	else
		response.SetResponse(answer);

	AppendResponse(response);

    if (memoryID != -1 && status == SUCCESS)
    {
		std::lock_guard<std::mutex> lk(memoryLock);
        chatMemories[memoryID] = memory;
    }
}

static void RequestsThread()
{
	while (running)
	{
		std::queue<AIRequest> curRequestes;

		{
			std::unique_lock<std::mutex> lk(requestLock);
			cvRequest.wait(lk, [&] 
				{ 
					if (!running)
						return true;

					curRequestes.swap(requestes);
					return true; 
				});
		}

		if (!running)
			break;

		while (!curRequestes.empty())
		{
			AIRequest request = curRequestes.front();
			/*
			if (globalParams.logMode >= LOG_VERBOSE)
			{
				std::string encPrompt = EncodingHelper::ConvertToWideByte(prompt, globalParams.encoding);
				logprintf("\nnew request: %s\n", encPrompt.c_str());
			}*/

			ExecuteRequest(request);

			curRequestes.pop();
		}
	}
}

void InitParams()
{
	/*
	botParams.model = "gpt-3.5-turbo"; //GPT!!! goooo
	botParams.botType = GPT; //0 - GPT, 1 - Gemini, 2 - LLAMA (https://groq.com/), 3 - DOUBAO
	botParams.encoding = W1252; //windows 1252
    botParams.timeoutMs = 15000;
    botParams.logMode = LOG_ERRORS; // errors only
	*/
}

void Start()
{
	running = true;
	requestsThread = std::thread(RequestsThread);
	requestsThread.detach();
}

void Stop()
{
	running = false;
	cvRequest.notify_one();
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	logprintf("ChatBot Plugin %s by SimoSbara loaded", PLUGIN_VERSION);
	
	Start();

	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	Stop();

	logprintf("ChatBot Plugin %s by SimoSbara unloaded", PLUGIN_VERSION);    
}

static cell AMX_NATIVE_CALL n_SetChatBotGlobalEncoding(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "SetChatBotGlobalEncoding"); //encoding int

	Encodings encoding = static_cast<Encodings>(params[1]);

	if (encoding == NUM_ENCODINGS)
		return 0;

	globalParams.encoding = encoding;

	return 0;
}

static cell AMX_NATIVE_CALL n_SetChatBotGlobalTimeout(AMX* amx, cell* params)
{
    CHECK_PARAMS(1, "SetChatBotGlobalTimeout");

	globalParams.timeout = static_cast<uint16_t>(params[1]);

    return 1;
}

static cell AMX_NATIVE_CALL n_SetChatBotGlobalLogMode(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "SetChatBotGlobalLogMode");

	LogModes mode = static_cast<LogModes>(params[1]);

	if (mode == NUM_LOG_MODES)
		return 0;

	globalParams.logMode = mode;

	return 1;
}

static cell AMX_NATIVE_CALL n_RequestToChatBot(AMX* amx, cell* params)
{
	char* pRequest = NULL;

	CHECK_PARAMS(3, "RequestToChatBot"); //chatid int, memoryid int, request string

	int chatbotID = static_cast<int>(params[1]);
	int memoryID = static_cast<int>(params[2]);
	amx_StrParam(amx, params[3], pRequest);

	if (!pRequest)
		return -1;

	int requestID = -1;

	{
		std::lock_guard<std::mutex> lock(requestLock);

		std::string requestStr = EncodingHelper::ConvertToUTF8(pRequest, globalParams.encoding);
		AIRequest newRequest(requestIncrementalID, chatbotID, memoryID, requestStr);

		requestes.push(newRequest);

		if (requestIncrementalID == INT_MAX)
			requestIncrementalID = 0;
		else
			requestIncrementalID++;

		requestID = newRequest.GetID();
	}

	cvRequest.notify_one();

	return requestID;
}

static cell AMX_NATIVE_CALL n_CreateChatBotMemory(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "CreateChatBotMemory"); //memoryid int
	int id = static_cast<int>(params[1]);

	{
		std::lock_guard<std::mutex> lock(memoryLock);

		if (chatMemories.find(id) != chatMemories.end())
			return 0;

		chatMemories[id] = ChatMemory();
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_ClearChatBotMemory(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "ClearChatBotMemory"); //memoryid int

	int id = static_cast<int>(params[1]);

	{
		std::lock_guard<std::mutex> lock(memoryLock);

		if (chatMemories.find(id) != chatMemories.end())
			return 0;

		chatMemories[id].Clear();
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_CreateChatBot(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "CreateChatBot"); //chatbotid int

	int id = static_cast<int>(params[1]);
	
	{
		std::lock_guard<std::mutex> lock(chatBotLock);

		if (chatBots.find(id) != chatBots.end())
			return 0;

		chatBots[id] = ChatBot();
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetChatBotSystemPrompt(AMX* amx, cell* params)
{
	char* pPrompt = NULL;

	CHECK_PARAMS(1, "SetChatBotSystemPrompt"); //chatbotid int, system prompt string
	int id = static_cast<int>(params[1]);
	amx_StrParam(amx, params[2], pPrompt);

	{
		std::lock_guard<std::mutex> lock(chatBotLock);

		if (chatBots.find(id) != chatBots.end())
			return 0;

		std::string systemPrompt;
		
		if (pPrompt)
			systemPrompt = EncodingHelper::ConvertToUTF8(pPrompt, globalParams.encoding);
		
		chatBots[id].SetSystemPrompt(systemPrompt);
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetChatBotModel(AMX* amx, cell* params)
{
	char* pModel = NULL;

	CHECK_PARAMS(1, "SetChatBotModel"); //chatbotid int, model string
	int id = static_cast<int>(params[1]);
	amx_StrParam(amx, params[2], pModel);

	if (!pModel)
		return 0;

	{
		std::lock_guard<std::mutex> lock(chatBotLock);

		if (chatBots.find(id) != chatBots.end())
			return 0;

		chatBots[id].SetModel(pModel);
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetChatBotEndPoint(AMX* amx, cell* params)
{
	char* pEndPoint = NULL;

	CHECK_PARAMS(1, "SetChatBotEndPoint"); //chatbotid int, endpoint string
	int id = static_cast<int>(params[1]);
	amx_StrParam(amx, params[2], pEndPoint);

	if (!pEndPoint)
		return 0;

	{
		std::lock_guard<std::mutex> lock(chatBotLock);

		if (chatBots.find(id) != chatBots.end())
			return 0;

		chatBots[id].SetEndPoint(pEndPoint);
	}

	return 1;
}

static cell AMX_NATIVE_CALL n_SetChatBotAPIKey(AMX* amx, cell* params)
{
	char* pKey = NULL;

	CHECK_PARAMS(1, "SetChatBotAPIKey"); //chatbotid int, api key string
	int id = static_cast<int>(params[1]);
	amx_StrParam(amx, params[2], pKey);

	if (!pKey)
		return 0;

	{
		std::lock_guard<std::mutex> lock(chatBotLock);

		if (chatBots.find(id) != chatBots.end())
			return 0;

		chatBots[id].SetAPIKey(pKey);
	}

	return 1;
}

AMX_NATIVE_INFO natives[] =
{
	{ "SetChatBotGlobalEncoding", n_SetChatBotGlobalEncoding },
    { "SetChatBotGlobalTimeout", n_SetChatBotGlobalTimeout },
    { "SetChatBotGlobalLogMode", n_SetChatBotGlobalLogMode },
	{ "RequestToChatBot", n_RequestToChatBot },
	{ "CreateChatBotMemory", n_CreateChatBotMemory },
	{ "ClearChatBotMemory", n_ClearChatBotMemory },
	{ "CreateChatBot", n_CreateChatBot },
	{ "SetChatBotSystemPrompt", n_SetChatBotSystemPrompt },
	{ "SetChatBotModel", n_SetChatBotModel },
	{ "SetChatBotEndPoint", n_SetChatBotEndPoint },
	{ "SetChatBotAPIKey", n_SetChatBotAPIKey },
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
	std::queue<AIResponse> curResponses;

	{
		std::lock_guard<std::mutex> lk(requestLock);
		curResponses.swap(responses);
	}

	while (!curResponses.empty())
	{
		AIResponse response = curResponses.front();
		curResponses.pop();

		Encodings encoding = globalParams.encoding;
		LogModes logMode = globalParams.logMode;

		//conversione a encoding originale
		std::string resp = EncodingHelper::ConvertToWideByte(response.GetResponse(), encoding);
		std::string prompt = EncodingHelper::ConvertToWideByte(response.GetPrompt(), encoding);

		if (logMode >= LOG_VERBOSE)
            logprintf("\nnew response: %s\n", resp.c_str());

		for (std::set<AMX*>::iterator a = interfaces.begin(); a != interfaces.end(); a++)
		{
			StatusCodes status = response.GetStatusCode();

			if (status != SUCCESS)
			{
				int amxIndex = 0;
				if (!amx_FindPublic(*a, "OnChatBotError", &amxIndex))
				{
					cell amxAddress = 0;
					amx_PushString(*a, &amxAddress, NULL, resp.c_str(), 0, 0);
					amx_Push(*a, response.GetRequestID());
					amx_Exec(*a, NULL, amxIndex);
					amx_Release(*a, amxAddress);
					continue;
				}
			}
			else
			{
				cell amxAddresses[2] = { 0 };
				int amxIndex = 0;

				if (!amx_FindPublic(*a, "OnChatBotResponse", &amxIndex))
				{
					//parametri al contrario
					amx_Push(*a, response.GetRequestID());
					amx_PushString(*a, &amxAddresses[0], NULL, resp.c_str(), 0, 0);
					amx_PushString(*a, &amxAddresses[1], NULL, prompt.c_str(), 0, 0);
					amx_Exec(*a, NULL, amxIndex);
					amx_Release(*a, amxAddresses[0]);
					amx_Release(*a, amxAddresses[1]);
				}
			}
		}
	}
}
