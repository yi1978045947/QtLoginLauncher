#include "client_config_model.h"

#include <cctype>
#include <map>

namespace qtlogin::config {
namespace {

struct JsonValue {
    enum class Type {
        Null,
        String,
        Number,
        Boolean,
        Array,
        Object,
    };

    Type type = Type::Null;
    std::string stringValue;
    int numberValue = 0;
    bool boolValue = false;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(JsonValue* value)
    {
        if (!value) {
            return false;
        }
        skipWhitespace();
        if (!parseValue(value)) {
            return false;
        }
        skipWhitespace();
        return index_ == text_.size();
    }

private:
    void skipWhitespace()
    {
        while (index_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[index_]))) {
            ++index_;
        }
    }

    bool parseValue(JsonValue* value)
    {
        skipWhitespace();
        if (index_ >= text_.size()) {
            return false;
        }
        switch (text_[index_]) {
        case '"':
            value->type = JsonValue::Type::String;
            return parseString(&value->stringValue);
        case '{':
            return parseObject(value);
        case '[':
            return parseArray(value);
        case 't':
            return parseLiteral("true", JsonValue::Type::Boolean, true, value);
        case 'f':
            return parseLiteral("false", JsonValue::Type::Boolean, false, value);
        case 'n':
            return parseLiteral("null", JsonValue::Type::Null, false, value);
        default:
            if (text_[index_] == '-' || std::isdigit(static_cast<unsigned char>(text_[index_]))) {
                return parseNumber(value);
            }
            return false;
        }
    }

    bool parseObject(JsonValue* value)
    {
        value->type = JsonValue::Type::Object;
        value->objectValue.clear();
        ++index_;
        skipWhitespace();
        if (consume('}')) {
            return true;
        }

        while (index_ < text_.size()) {
            std::string key;
            if (!parseString(&key)) {
                return false;
            }
            skipWhitespace();
            if (!consume(':')) {
                return false;
            }
            JsonValue child;
            if (!parseValue(&child)) {
                return false;
            }
            value->objectValue[key] = std::move(child);
            skipWhitespace();
            if (consume('}')) {
                return true;
            }
            if (!consume(',')) {
                return false;
            }
            skipWhitespace();
        }
        return false;
    }

    bool parseArray(JsonValue* value)
    {
        value->type = JsonValue::Type::Array;
        value->arrayValue.clear();
        ++index_;
        skipWhitespace();
        if (consume(']')) {
            return true;
        }

        while (index_ < text_.size()) {
            JsonValue child;
            if (!parseValue(&child)) {
                return false;
            }
            value->arrayValue.push_back(std::move(child));
            skipWhitespace();
            if (consume(']')) {
                return true;
            }
            if (!consume(',')) {
                return false;
            }
            skipWhitespace();
        }
        return false;
    }

    bool parseString(std::string* output)
    {
        if (!consume('"')) {
            return false;
        }
        std::string result;
        while (index_ < text_.size()) {
            const char ch = text_[index_++];
            if (ch == '"') {
                *output = result;
                return true;
            }
            if (ch != '\\') {
                result.push_back(ch);
                continue;
            }
            if (index_ >= text_.size()) {
                return false;
            }
            const char escaped = text_[index_++];
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u':
                if (!parseUnicodeEscape(&result)) {
                    return false;
                }
                break;
            default:
                return false;
            }
        }
        return false;
    }

    bool parseUnicodeEscape(std::string* output)
    {
        if (index_ + 4 > text_.size()) {
            return false;
        }
        unsigned int codepoint = 0;
        for (int i = 0; i < 4; ++i) {
            const char ch = text_[index_++];
            codepoint <<= 4;
            if (ch >= '0' && ch <= '9') {
                codepoint += static_cast<unsigned int>(ch - '0');
            } else if (ch >= 'a' && ch <= 'f') {
                codepoint += static_cast<unsigned int>(ch - 'a' + 10);
            } else if (ch >= 'A' && ch <= 'F') {
                codepoint += static_cast<unsigned int>(ch - 'A' + 10);
            } else {
                return false;
            }
        }
        appendUtf8(codepoint, output);
        return true;
    }

    static void appendUtf8(unsigned int codepoint, std::string* output)
    {
        if (codepoint <= 0x7F) {
            output->push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            output->push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
            output->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            output->push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
            output->push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            output->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    bool parseLiteral(const char* literal, JsonValue::Type type, bool boolValue, JsonValue* value)
    {
        const std::string expected(literal);
        if (text_.compare(index_, expected.size(), expected) != 0) {
            return false;
        }
        index_ += expected.size();
        value->type = type;
        value->boolValue = boolValue;
        return true;
    }

    bool parseNumber(JsonValue* value)
    {
        const size_t begin = index_;
        if (text_[index_] == '-') {
            ++index_;
        }
        while (index_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[index_]))) {
            ++index_;
        }
        if (begin == index_ || (text_[begin] == '-' && begin + 1 == index_)) {
            return false;
        }
        value->type = JsonValue::Type::Number;
        value->numberValue = std::atoi(text_.substr(begin, index_ - begin).c_str());
        return true;
    }

    bool consume(char expected)
    {
        if (index_ >= text_.size() || text_[index_] != expected) {
            return false;
        }
        ++index_;
        return true;
    }

    const std::string& text_;
    size_t index_ = 0;
};

const JsonValue* objectField(const JsonValue& value, const std::string& key)
{
    if (value.type != JsonValue::Type::Object) {
        return nullptr;
    }
    const auto it = value.objectValue.find(key);
    return it == value.objectValue.end() ? nullptr : &it->second;
}

std::string stringField(const JsonValue& object, const std::string& key)
{
    const JsonValue* value = objectField(object, key);
    return value && value->type == JsonValue::Type::String ? value->stringValue : std::string();
}

bool boolField(const JsonValue& object, const std::string& key)
{
    const JsonValue* value = objectField(object, key);
    if (!value) {
        return false;
    }
    if (value->type == JsonValue::Type::Boolean) {
        return value->boolValue;
    }
    if (value->type == JsonValue::Type::Number) {
        return value->numberValue == 1;
    }
    if (value->type == JsonValue::Type::String) {
        return value->stringValue == "1" || value->stringValue == "true" || value->stringValue == "True";
    }
    return false;
}

std::string flagStringField(const JsonValue& object, const std::string& key)
{
    return boolField(object, key) ? "1" : "0";
}

bool loginMethodFromString(const std::string& value, LoginMethod* method)
{
    if (value == "pwdLogin") {
        *method = LoginMethod::Password;
        return true;
    }
    if (value == "pushMessageLogin") {
        *method = LoginMethod::PushMessage;
        return true;
    }
    if (value == "codeKeyLogin") {
        *method = LoginMethod::QrCode;
        return true;
    }
    if (value == "safePhoneSmsLogin") {
        *method = LoginMethod::SafePhoneSms;
        return true;
    }
    return false;
}

void parseActivationHint(const std::string& activationCodeHint, ClientConfigModel* config)
{
    if (activationCodeHint.empty()) {
        return;
    }
    JsonValue activationRoot;
    if (!JsonParser(activationCodeHint).parse(&activationRoot) || activationRoot.type != JsonValue::Type::Object) {
        return;
    }
    config->activationCodeTitle = stringField(activationRoot, "activationCodeTitle");
    config->activationCodeTips = stringField(activationRoot, "activationCodeTips");
}

}

ClientConfigModel parseClientConfigJson(const std::string& json)
{
    ClientConfigModel config;
    JsonValue root;
    if (!JsonParser(json).parse(&root) || root.type != JsonValue::Type::Object) {
        config.parseOk = false;
        return config;
    }

    config.voiceSendTip = stringField(root, "voiceSendTip");
    config.voiceRemindTip = stringField(root, "voiceRemindTip");
    config.supportMobileAccountOnly = boolField(root, "supportMobileAccountOnly");
    config.supportMobileAccountOnlyTip = stringField(root, "supportMobileAccountOnlyTip");
    config.forbiddenStaticPwd = boolField(root, "forbiddenStaticPwd");
    config.forbiddenStaticPwdTip = stringField(root, "forbiddenStaticPwdTip");
    config.realNameNeedTwiceConfirm = flagStringField(root, "realNameNeedTwiceConfirm");
    config.realNameTwiceConfirmPrompt = stringField(root, "realNameTwiceConfirmPrompt");
    parseActivationHint(stringField(root, "activationCodeHint"), &config);

    if (const JsonValue* methods = objectField(root, "loginMethodList")) {
        if (methods->type == JsonValue::Type::Array) {
            for (const JsonValue& value : methods->arrayValue) {
                if (value.type != JsonValue::Type::String) {
                    continue;
                }
                LoginMethod method = LoginMethod::Password;
                if (loginMethodFromString(value.stringValue, &method)) {
                    config.loginMethodList.push_back(method);
                }
            }
        }
    }

    return config;
}

bool hasLoginMethod(const ClientConfigModel& config, LoginMethod method)
{
    for (LoginMethod item : config.loginMethodList) {
        if (item == method) {
            return true;
        }
    }
    return false;
}

}
