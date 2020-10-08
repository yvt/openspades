#include <requestList.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Json::Value serverInfoM(){
	auto curl = curl_easy_init();
	Json::Value coisado;
	if(curl){
		SPADES_SETTING(cl_serverListUrl);
		SPADES_SETTING(cg_lastQuickConnectHost);

		std::string buffer;
		Json::Value jsonData;
		Json::Reader jsonReader;

		curl_easy_setopt(curl, CURLOPT_USERAGENT, "1.0.0");
		curl_easy_setopt(curl, CURLOPT_URL, cl_serverListUrl.CString());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if(jsonReader.parse(buffer, jsonData, false)){

			for(Json::Value::iterator it = jsonData.begin(); it != jsonData.end(); ++it){
				Json::Value &obj = *it;
				if(obj["identifier"].asString() == cg_lastQuickConnectHost){
					coisado = obj;
				}
			}
		}
	}

	return coisado;
}