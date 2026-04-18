// Defines

#define CHATBOT_UTF8			0
#define CHATBOT_W1252			1
#define CHATBOT_GB2312			2
#define CHATBOT_W1251			3

#define CHATBOT_LOG_SILENT		0
#define CHATBOT_LOG_ERRORS		1
#define CHATBOT_LOG_VERBOSE		2

// Natives

native SetChatBotGlobalEncoding(encoding);
native SetChatBotGlobalTimeout(ms);
native SetChatBotGlobalLogMode(mode);

native CreateChatBotMemory(memoryid);
native ClearChatBotMemory(memoryid);

native CreateChatBot(chatbotid);

native RequestToChatBot(chatbotid, memoryid, const prompt[]);

native SetChatBotSystemPrompt(chatbotid, const systemPrompt[]);
native SetChatBotModel(chatbotid, const model[]);
native SetChatBotEndPoint(chatbotid, const endpoint[]);
native SetChatBotAPIKey(chatbotid, const apiKey[]);

// Callbacks

forward OnChatBotResponse(prompt[], response[], requestid);
forward OnChatBotError(requestid, const error[]);
