#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <set>

#include "EncodingHelper.hpp"
#include "SampHelper.h"
#include "ChatBotHelper.h"

static std::queue<AIRequest> requestes;
static std::queue<AIResponse> responses;

static std::map<int, ChatMemory> chatMemories;
static ChatBotParams botParams;

static std::mutex requestLock;
static std::mutex responseLock;
static std::mutex paramsLock;
static std::mutex memoryLock;

static std::thread requestsThread;
static std::atomic<bool> running;

static void DoRequest(std::string prompt, int id)
{
	std::string answer;

	paramsLock.lock();
	ChatBotParams curParams = botParams;
	paramsLock.unlock();

	ChatMemory memory;

	//lo trovo?
	memoryLock.lock();
	if (chatMemories.find(id) != chatMemories.end())
		memory = chatMemories[id];
	memoryLock.unlock();

	if (ChatBotHelper::DoRequest(answer, prompt, curParams, memory))
	{
		AIResponse response(id, prompt, answer);

		responseLock.lock();
		responses.push(response);
		responseLock.unlock();

		//aggiorno la memoria
		memoryLock.lock();
		chatMemories[id] = memory;
		memoryLock.unlock();
	}
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

void InitParams()
{
	botParams.model = "gpt-3.5-turbo"; //GPT!!! goooo
	botParams.botType = GPT; //0 - GPT, 1 - Gemini, 2 - LLAMA (https://groq.com/)
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
	InitParams();

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

static cell AMX_NATIVE_CALL n_ClearMemory(AMX* amx, cell* params)
{
	CHECK_PARAMS(1, "ClearMemory"); //id int, request string

	int id = static_cast<int>(params[1]);

	memoryLock.lock();
	if (chatMemories.find(id) != chatMemories.end())
	{
		chatMemories[id].Clear();
		memoryLock.unlock();
		
		return 1;
	}

	memoryLock.unlock();

	return 0;
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
		paramsLock.lock();
		botParams.botType = type;
		paramsLock.unlock();

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
		paramsLock.lock();
		botParams.apikey = std::string(pKey);
		paramsLock.unlock();

		return 1;
	}

	return 0;
}

static cell AMX_NATIVE_CALL n_SetSystemPrompt(AMX* amx, cell* params)
{
	char* pPrompt = NULL;

	CHECK_PARAMS(1, "SetSystemPrompt"); //system prompt string
	amx_StrParam(amx, params[1], pPrompt);

	paramsLock.lock();

	if (pPrompt)
		botParams.systemPrompt = std::string(pPrompt);
	else
		botParams.systemPrompt.clear();

	paramsLock.unlock();

	return 1;
}

static cell AMX_NATIVE_CALL n_SetModel(AMX* amx, cell* params)
{
	char* pModel = NULL;

	CHECK_PARAMS(1, "SetModel"); //chatbot model string
	amx_StrParam(amx, params[1], pModel);

	if (pModel)
	{
		paramsLock.lock();
		botParams.model = std::string(pModel);
		paramsLock.unlock();

		return 1;
	}

	return 0;
}

AMX_NATIVE_INFO natives[] =
{
	{ "ClearMemory", n_ClearMemory },
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
