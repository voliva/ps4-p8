
#include <string>
#include <vector>

void http_init();
std::vector<unsigned char> http_get(std::string url);
std::string http_get_string(std::string url);
