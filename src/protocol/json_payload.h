#pragma once

#include <map>
#include <string>

namespace qtlogin::protocol {

std::string encodeJsonPayload(const std::map<std::string, std::string>& fields);
bool decodeJsonPayload(const std::string& payload, std::map<std::string, std::string>* fields);

}
