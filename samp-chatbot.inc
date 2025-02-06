// Defines

#define CHAT_GPT	0
#define GEMINI_AI	1
#define LLAMA		2
#define DOUBAO		3
#define DEEPSEEK	4

#define W1252		0
#define GB2312		1
#define W1251		2

// Natives

native SetChatBotEncoding(encoding);
native ClearMemory(id);
native RequestToChatBot(const prompt[], id);
native SelectChatBot(type);
native SetAPIKey(const apiKey[]);
native SetSystemPrompt(const systemPrompt[]);
native SetModel(const model[]);

// Callbacks

forward OnChatBotResponse(prompt[], response[], id);
