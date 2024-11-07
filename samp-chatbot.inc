// Defines

#define CHAT_GPT	0
#define GEMINI_AI	1
#define LLAMA		2

// Natives

native RequestToChatBot(const prompt[], id);
native SelectChatBot(type);
native SetAPIKey(const apiKey[]);
native SetSystemPrompt(const systemPrompt[]);
native SetModel(const model[]);

// Callbacks

forward OnChatBotResponse(prompt[], response[], id);
