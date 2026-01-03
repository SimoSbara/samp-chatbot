// Defines

#define CHAT_GPT	0
#define GEMINI_AI	1
#define LLAMA		2
#define DOUBAO		3
#define DEEPSEEK	4

#define UTF8		0
#define W1252	    1
#define GB2312		2
#define W1251		3

#define CHATBOT_LOG_SILENT  0
#define CHATBOT_LOG_ERRORS  1
#define CHATBOT_LOG_VERBOSE 2

// Natives

native SetChatBotEncoding(encoding);
native SetChatBotTimeout(ms);
native SetChatBotLogMode(mode);
native ClearMemory(id);
native RequestToChatBot(const prompt[], id);
native SelectChatBot(type);
native GetChatBotType();
native SetAPIKey(const apiKey[]);
native SetSystemPrompt(const systemPrompt[]);
native SetModel(const model[]);

// Callbacks

forward OnChatBotResponse(prompt[], response[], id);
forward OnChatBotError(id, const error[]);
