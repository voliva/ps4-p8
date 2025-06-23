#include "http.h"
#include "log.h"

#define DEBUGLOG Http_DEBUGLOG
Log DEBUGLOG = logger.log("HTTP");

#ifdef __PS4__

#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>

// Function based from @bucanero's https://github.com/bucanero/apollo-ps4/blob/main/source/http.c - Code under GPLv3
#define NET_POOLSIZE 	(4 * 1024)
#define HTTP_SUCCESS 	1
#define HTTP_FAILED	 	0
#define HTTP_USER_AGENT "Mozilla/5.0 (PLAYSTATION 4; 1.00)"
int libnetMemId = 0, libhttpCtxId = 0, libsslCtxId = 0;
void http_init()
{
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)
        DEBUGLOG << "Failed initializing net module" << ENDL;

	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0)
        DEBUGLOG << "Failed initializing http module" << ENDL;

    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)
        DEBUGLOG << "Failed initializing ssl module" << ENDL;

    if (libnetMemId == 0 || libhttpCtxId == 0 || libsslCtxId == 0)
    {
        int ret = sceNetInit();
        ret = sceNetPoolCreate("netPool", NET_POOLSIZE, 0);
        if (ret < 0) {
            DEBUGLOG << "sceNetPoolCreate() error: " << ret << ENDL;
            return;
        }
        libnetMemId = ret;

        ret = sceSslInit(SSL_POOLSIZE);
        if (ret < 0) {
            DEBUGLOG << "sceSslInit() error: " << ret << ENDL;
            return;
        }
        libsslCtxId = ret;

        ret = sceHttpInit(libnetMemId, libsslCtxId, LIBHTTP_POOLSIZE);
        if (ret < 0) {
            DEBUGLOG << "sceHttpInit() error: " << ret << ENDL;
            return;
        }
        libhttpCtxId = ret;
    }
}

int skipSslCallback(int libsslId, unsigned int verifyErr, void* const sslCert[], int certNum, void* userArg)
{
	return HTTP_SUCCESS;
}

// Function based from @bucanero's https://github.com/bucanero/apollo-ps4/blob/main/source/http.c - Code under GPLv3
std::vector<unsigned char> http_get(std::string url)
{
	int tpl = 0, conn = 0, req = 0;
	std::vector<unsigned char> result;

	tpl = sceHttpCreateTemplate(libhttpCtxId, HTTP_USER_AGENT, ORBIS_HTTP_VERSION_1_1, 1);
	if (tpl < 0) {
		DEBUGLOG << "sceHttpCreateConnectionWithURL() error: " << tpl << ENDL;
		return result;
	}
	
	int ret = sceHttpsSetSslCallback(tpl, skipSslCallback, NULL);
	if (ret < 0) {
		DEBUGLOG << "sceHttpsSetSslCallback() error: " << ret << ENDL;
		goto close_http;
	}

	conn = sceHttpCreateConnectionWithURL(tpl, url.c_str(), 1);
	if (conn < 0) {
		DEBUGLOG << "sceHttpCreateConnectionWithURL() error: " << ret << ENDL;
		goto close_http;
	}

	req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_GET, url.c_str(), 0);
	if (req < 0) {
		DEBUGLOG << "sceHttpCreateRequestWithURL() error: " << ret << ENDL;
		goto close_http;
	}

	ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0) {
		DEBUGLOG << "sceHttpSendRequest() error: " << ret << ENDL;
		goto close_http;
	}

	int32_t statusCode;
	ret = sceHttpGetStatusCode(req, &statusCode);
	if (ret < 0) {
		DEBUGLOG << "sceHttpGetStatusCode() error: " << ret << ENDL;
		goto close_http;
	}

	if (statusCode != 200)
	{
		if (statusCode == 404) {
			DEBUGLOG << "404 URL not found: " << url << ENDL;
		} else {
			DEBUGLOG << "Unknown status code: " << statusCode << ENDL;
		}

		goto close_http;
	}

	{
		unsigned char buffer[8 * 1024];
		int read, total_bytes = 0;
		while ((read = sceHttpReadData(req, buffer, sizeof(buffer))) > 0) {
			int offset = total_bytes;
			total_bytes += read;
			result.resize(total_bytes);
			memcpy(&result[offset], buffer, read);
		}
	}

close_http:
	if (req > 0) {
		ret = sceHttpDeleteRequest(req);
		if (ret < 0) {
			DEBUGLOG << "sceHttpDeleteRequest() error: " << ret << ENDL;
		}
	}

	if (conn > 0) {
		ret = sceHttpDeleteConnection(conn);
		if (ret < 0) {
			DEBUGLOG << "sceHttpDeleteConnection() error: " << ret << ENDL;
		}
	}

	if (tpl > 0) {
		ret = sceHttpDeleteTemplate(tpl);
		if (ret < 0) {
			DEBUGLOG << "sceHttpDeleteTemplate() error: " << ret << ENDL;
		}
	}

	return result;
}

#else

#include <curl/curl.h>
#include <cstring>

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
