#include "ChatBotHelper.h"
#include "EncodingHelper.hpp"
#include "SampHelper.h"

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "libcurl_debug.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif

#endif

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
	size_t totalSize = size * nmemb;
	response->append((char*)contents, totalSize);
	return totalSize;
}

curl_slist* ChatBotHelper::GetHeader(const ChatBotParams& params)
{
	curl_slist* headers = NULL;

	switch (params.botType)
	{
	case GPT:
	case LLAMA:
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + params.apikey).c_str());
		break;
	case GEMINI:
		headers = curl_slist_append(headers, "Content-Type: application/json");
		break;
	}

	return headers;
}

std::string ChatBotHelper::GetURL(const ChatBotParams& params)
{
	switch (params.botType)
	{
	case GPT:
		return "https://api.openai.com/v1/chat/completions";
	case GEMINI:
		return "https://generativelanguage.googleapis.com/v1beta/models/" + params.model + ":generateContent?key=" + params.apikey;
	case LLAMA:
		return "https://api.groq.com/openai/v1/chat/completions";
	}

	return "";
}

json ChatBotHelper::CreateRequestDocument(const std::string request, const ChatBotParams& params, const ChatMemory& memory)
{
	json requestDoc;
	int index = 0;

	switch (params.botType)
	{
	case LLAMA:
	case GPT:
	{
		requestDoc["model"] = params.model;

		if (!params.systemPrompt.empty())
		{
			requestDoc["messages"][index]["role"] = "system";
			requestDoc["messages"][index]["content"] = params.systemPrompt;
			index++;
		}

		//aggiungo memoria
		for (int i = 0; i < memory.GetSize(); i++)
		{
			Message message = memory.GetMessageFromIndex(i);

			requestDoc["messages"][index]["role"] = (message.isBot) ? "assistant" : "user";
			requestDoc["messages"][index]["content"] = message.msg;

			index++;
		}

		//ultima richiesta
		requestDoc["messages"][index]["role"] = "user";
		requestDoc["messages"][index]["content"] = request;
		requestDoc["temperature"] = 0;
	}
	break;
	case GEMINI:
	{
		if (!params.systemPrompt.empty())
			requestDoc["system_instruction"]["parts"]["text"] = params.systemPrompt;

		//aggiungo memoria
		for (int i = 0; i < memory.GetSize(); i++)
		{
			Message message = memory.GetMessageFromIndex(i);

			requestDoc["contents"][index]["role"] = (message.isBot) ? "model" : "user";
			requestDoc["contents"][index]["parts"][0]["text"] = message.msg;

			index++;
		}

		requestDoc["contents"][index]["role"] = "user";
		requestDoc["contents"][index]["parts"][0]["text"] = request;
	}
	break;
	}

	return requestDoc;
}

bool ChatBotHelper::DoRequest(std::string& response, const std::string request, const ChatBotParams& params, ChatMemory& memory)
{
	CURL* curl = curl_easy_init();

	if (curl)
	{
		std::string curlResponse;

		curl_slist* headers = GetHeader(params);
		std::string url = GetURL(params);

		json requestDataDoc = CreateRequestDocument(request, params, memory);

		std::string requestDataStr = requestDataDoc.dump();

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestDataStr.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestDataStr.length());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResponse);

		CURLcode res = curl_easy_perform(curl);

		if (res == CURLE_OK)
		{
			nlohmann::json jresponse = nlohmann::json::parse(curlResponse);
			response = GetBotAnswer(params.botType, jresponse);

			//aggiungo nella memoria
			memory.AddUserMessage(request);
			memory.AddBotMessage(response);
		}
		else
			logprintf("\n\nChat Bot Request Failed! Error: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		return res == CURLE_OK;
	}

	curl_easy_cleanup(curl);

	return false;
}

std::string ChatBotHelper::GetBotAnswer(int botType, nlohmann::json response)
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

			switch (botType)
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
