#include <string>

std::string base64_encode(const std::string& data);
std::string base64_encode(const char* data, unsigned int len);
std::string base64_decode(std::string const& s);
