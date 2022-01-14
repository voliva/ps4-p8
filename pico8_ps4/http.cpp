#include "http.h"
#include "log.h"

#define DEBUGLOG Http_DEBUGLOG
Log DEBUGLOG = logger.log("HTTP");

#ifdef __PS4__

void http_init()
{
}

std::vector<unsigned char> http_get(std::string url)
{
    return std::vector<unsigned char>();
}

#else

#include <curl/curl.h>

CURL* curl;


void http_init()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}


static size_t curl_write_fn(void* buffer, size_t size, size_t nmemb, void* stream)
{
    std::vector<unsigned char>* out = (std::vector<unsigned char>*)stream;
    int offset = out->size();
    out->resize(offset + size * nmemb);
    memcpy(&(*out)[offset], buffer, size * nmemb);
    return size * nmemb;
}

std::vector<unsigned char> http_get(std::string url)
{
    std::vector<unsigned char> result;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_fn);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    CURLcode res = curl_easy_perform(curl);
    
    if(res != CURLE_OK) {
        DEBUGLOG << "Error: " << res << " - " << curl_easy_strerror(res) << ENDL;
    }

    curl_easy_cleanup(curl);

    return result;
}

#endif


std::string http_get_string(std::string url)
{
    std::vector<unsigned char> result = http_get(url);

    if (result.size() == 0) {
        DEBUGLOG << "Received empty response for " << url << ENDL;
        return "";
    }
    else {
        return std::string((char*)&result[0], result.size());
    }
}
