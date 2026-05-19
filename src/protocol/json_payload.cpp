#include "json_payload.h"

namespace qtlogin::protocol {
namespace {

std::string escapeJson(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            result += ch;
            break;
        }
    }
    return result;
}

bool parseJsonString(const std::string& text, size_t* pos, std::string* value)
{
    if (*pos >= text.size() || text[*pos] != '"') {
        return false;
    }
    ++(*pos);
    std::string result;
    while (*pos < text.size()) {
        const char ch = text[(*pos)++];
        if (ch == '"') {
            *value = result;
            return true;
        }
        if (ch == '\\') {
            if (*pos >= text.size()) {
                return false;
            }
            const char esc = text[(*pos)++];
            switch (esc) {
            case '\\': result += '\\'; break;
            case '"': result += '"'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            default: return false;
            }
        } else {
            result += ch;
        }
    }
    return false;
}

void skipWhitespace(const std::string& text, size_t* pos)
{
    while (*pos < text.size()) {
        const char ch = text[*pos];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        ++(*pos);
    }
}

}

std::string encodeJsonPayload(const std::map<std::string, std::string>& fields)
{
    std::string result = "{";
    bool first = true;
    for (const auto& [key, value] : fields) {
        if (!first) {
            result += ",";
        }
        first = false;
        result += "\"";
        result += escapeJson(key);
        result += "\":\"";
        result += escapeJson(value);
        result += "\"";
    }
    result += "}";
    return result;
}

bool decodeJsonPayload(const std::string& payload, std::map<std::string, std::string>* fields)
{
    if (!fields) {
        return false;
    }
    fields->clear();

    size_t pos = 0;
    skipWhitespace(payload, &pos);
    if (pos >= payload.size() || payload[pos++] != '{') {
        return false;
    }

    skipWhitespace(payload, &pos);
    if (pos < payload.size() && payload[pos] == '}') {
        return true;
    }

    while (pos < payload.size()) {
        std::string key;
        std::string value;
        skipWhitespace(payload, &pos);
        if (!parseJsonString(payload, &pos, &key)) {
            return false;
        }
        skipWhitespace(payload, &pos);
        if (pos >= payload.size() || payload[pos++] != ':') {
            return false;
        }
        skipWhitespace(payload, &pos);
        if (!parseJsonString(payload, &pos, &value)) {
            return false;
        }
        (*fields)[key] = value;
        skipWhitespace(payload, &pos);
        if (pos >= payload.size()) {
            return false;
        }
        if (payload[pos] == '}') {
            ++pos;
            skipWhitespace(payload, &pos);
            return pos == payload.size();
        }
        if (payload[pos++] != ',') {
            return false;
        }
    }
    return false;
}

}
