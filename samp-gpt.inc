// Natives

native RequestToGPT(const prompt[], playerid);
native SetAPIKey(const apiKey[]);
native SetSystemPrompt(const systemPrompt[]);

// Callbacks

forward OnGPTResponse(response[], playerid);
