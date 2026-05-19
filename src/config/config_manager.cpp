#include "config_manager.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cwctype>
#include <cstdlib>
#include <string>
#include <vector>

#include <windows.h>

#include "process_util.h"
#include "string_util.h"

namespace qtlogin::config {
namespace {

using ParsedAttributeMap = std::map<std::wstring, std::wstring>;

const char* kDefaultUserInfo =
    "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\r\n"
    "<UserInfo>\r\n"
    "<Alert value=\"True\" />\r\n"
    "<LoginMode value=\"0\" />\r\n"
    "<LoginOption value=\"False\" />\r\n"
    "<Accounts />\r\n"
    "<LauncherDatas />\r\n"
    "<Tgts />\r\n"
    "</UserInfo>";

bool fileExists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool readFileBytes(const std::wstring& path, std::string* output)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 16 * 1024 * 1024) {
        CloseHandle(file);
        return false;
    }

    std::string data(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    const BOOL ok = data.empty() || ReadFile(file, data.data(), static_cast<DWORD>(data.size()), &read, nullptr);
    CloseHandle(file);
    if (!ok || read != data.size()) {
        return false;
    }

    if (data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        data.erase(0, 3);
    }
    *output = data;
    return true;
}

bool writeFileBytes(const std::wstring& path, const std::string& text)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const BOOL ok = text.empty() || WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    CloseHandle(file);
    return ok && written == text.size();
}

const wchar_t* fileStem(ConfigFile file)
{
    switch (file) {
    case ConfigFile::ClientInfo:
        return L"clientinfo";
    case ConfigFile::System:
        return L"config";
    case ConfigFile::Version:
        return L"version";
    case ConfigFile::UserInfo:
        return L"userinfo";
    case ConfigFile::Browser:
        return L"browser";
    case ConfigFile::Widget:
        return L"widget";
    case ConfigFile::SkinType:
        return L"skintype";
    }
    return L"config";
}

const char* rootNodeName(ConfigFile file)
{
    switch (file) {
    case ConfigFile::ClientInfo:
        return "ClientInfo";
    case ConfigFile::System:
        return "Config";
    case ConfigFile::Version:
        return "VersionInfo";
    case ConfigFile::UserInfo:
        return "UserInfo";
    case ConfigFile::Browser:
        return "Browser";
    case ConfigFile::Widget:
        return "Widget";
    case ConfigFile::SkinType:
        return "SkinType";
    }
    return "Config";
}

std::wstring writableConfigPath(const std::wstring& rootDirectory, ConfigFile file)
{
    const std::wstring configDir = common::joinPath(rootDirectory, L"config");
    const std::wstring stem = fileStem(file);
    const std::wstring xmlPath = common::joinPath(configDir, stem + L".xml");
    const std::wstring cfgPath = common::joinPath(configDir, stem + L".cfg");
    const std::wstring rootXmlPath = common::joinPath(rootDirectory, stem + L".xml");
    const std::wstring rootCfgPath = common::joinPath(rootDirectory, stem + L".cfg");

    if (fileExists(xmlPath)) {
        return xmlPath;
    }
    if (fileExists(cfgPath)) {
        return cfgPath;
    }
    if (fileExists(rootXmlPath)) {
        return rootXmlPath;
    }
    if (fileExists(rootCfgPath)) {
        return rootCfgPath;
    }

    CreateDirectoryW(configDir.c_str(), nullptr);
    return xmlPath;
}

bool isNameChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) ||
        ch == '_' || ch == '-' || ch == ':' || ch == '.';
}

void skipSpaces(const std::string& text, size_t* index)
{
    while (*index < text.size() && std::isspace(static_cast<unsigned char>(text[*index]))) {
        ++(*index);
    }
}

std::string readName(const std::string& text, size_t* index)
{
    const size_t begin = *index;
    while (*index < text.size() && isNameChar(text[*index])) {
        ++(*index);
    }
    return text.substr(begin, *index - begin);
}

std::string htmlEntityDecode(std::string value)
{
    struct Replacement {
        const char* from;
        const char* to;
    };

    static const Replacement replacements[] = {
        {"&quot;", "\""},
        {"&apos;", "'"},
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
    };

    for (const Replacement& item : replacements) {
        size_t pos = 0;
        while ((pos = value.find(item.from, pos)) != std::string::npos) {
            value.replace(pos, std::strlen(item.from), item.to);
            pos += std::strlen(item.to);
        }
    }
    return value;
}

std::string htmlEntityEncode(std::string value)
{
    struct Replacement {
        const char* from;
        const char* to;
    };

    static const Replacement replacements[] = {
        {"&", "&amp;"},
        {"\"", "&quot;"},
        {"'", "&apos;"},
        {"<", "&lt;"},
        {">", "&gt;"},
    };

    for (const Replacement& item : replacements) {
        size_t pos = 0;
        while ((pos = value.find(item.from, pos)) != std::string::npos) {
            value.replace(pos, std::strlen(item.from), item.to);
            pos += std::strlen(item.to);
        }
    }
    return value;
}

bool isTagNameTerminator(char ch)
{
    return ch == '>' || ch == '/' || std::isspace(static_cast<unsigned char>(ch));
}

size_t findStartTag(const std::string& xml, const std::string& nodeName)
{
    const std::string pattern = "<" + nodeName;
    size_t pos = 0;
    while ((pos = xml.find(pattern, pos)) != std::string::npos) {
        const size_t next = pos + pattern.size();
        if (next < xml.size() && isTagNameTerminator(xml[next])) {
            return pos;
        }
        pos = next;
    }
    return std::string::npos;
}

std::string upsertXmlAttribute(
    std::string xml,
    const std::string& rootName,
    const std::string& nodeName,
    const std::string& attribute,
    const std::string& newValue)
{
    if (xml.empty()) {
        xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<" + rootName + ">\r\n</" + rootName + ">\r\n";
    }

    const std::string encodedValue = htmlEntityEncode(newValue);
    const size_t nodeStart = findStartTag(xml, nodeName);
    if (nodeStart != std::string::npos) {
        const size_t tagEnd = xml.find('>', nodeStart);
        if (tagEnd == std::string::npos) {
            return xml;
        }

        const std::string attrPattern = attribute + "=";
        size_t attrPos = xml.find(attrPattern, nodeStart);
        if (attrPos != std::string::npos && attrPos < tagEnd) {
            size_t valuePos = attrPos + attrPattern.size();
            while (valuePos < tagEnd && std::isspace(static_cast<unsigned char>(xml[valuePos]))) {
                ++valuePos;
            }
            if (valuePos < tagEnd && (xml[valuePos] == '"' || xml[valuePos] == '\'')) {
                const char quote = xml[valuePos];
                const size_t valueBegin = valuePos + 1;
                const size_t valueEnd = xml.find(quote, valueBegin);
                if (valueEnd != std::string::npos && valueEnd <= tagEnd) {
                    xml.replace(valueBegin, valueEnd - valueBegin, encodedValue);
                    return xml;
                }
            }
        }

        size_t insertPos = tagEnd;
        if (insertPos > nodeStart && xml[insertPos - 1] == '/') {
            --insertPos;
        }
        xml.insert(insertPos, " " + attribute + "=\"" + encodedValue + "\"");
        return xml;
    }

    const std::string closingRoot = "</" + rootName + ">";
    const size_t rootClose = xml.rfind(closingRoot);
    const std::string nodeText = "\r\n<" + nodeName + " " + attribute + "=\"" + encodedValue + "\" />\r\n";
    if (rootClose != std::string::npos) {
        xml.insert(rootClose, nodeText);
        return xml;
    }

    return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<" + rootName + ">" + nodeText + "</" + rootName + ">\r\n";
}

bool equalsIgnoreCase(std::wstring left, std::wstring right)
{
    std::transform(left.begin(), left.end(), left.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    std::transform(right.begin(), right.end(), right.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return left == right;
}

std::wstring trimAccountName(const std::wstring& value)
{
    const wchar_t* spaces = L" \t\r\n";
    const size_t begin = value.find_first_not_of(spaces);
    if (begin == std::wstring::npos) {
        return std::wstring();
    }
    const size_t end = value.find_last_not_of(spaces);
    return value.substr(begin, end - begin + 1);
}

ParsedAttributeMap readStartTagAttributes(const std::string& xml, size_t tagStart, size_t* tagEndOut)
{
    ParsedAttributeMap attrs;
    const size_t tagEnd = xml.find('>', tagStart);
    if (tagEnd == std::string::npos) {
        if (tagEndOut) {
            *tagEndOut = std::string::npos;
        }
        return attrs;
    }
    if (tagEndOut) {
        *tagEndOut = tagEnd;
    }

    size_t pos = tagStart + 1;
    readName(xml, &pos);
    while (pos < tagEnd) {
        if (xml[pos] == '/') {
            ++pos;
            continue;
        }
        skipSpaces(xml, &pos);
        if (pos >= tagEnd || xml[pos] == '/' || xml[pos] == '>') {
            continue;
        }

        const std::string attrNameUtf8 = readName(xml, &pos);
        skipSpaces(xml, &pos);
        if (attrNameUtf8.empty() || pos >= tagEnd || xml[pos] != '=') {
            continue;
        }
        ++pos;
        skipSpaces(xml, &pos);
        if (pos >= tagEnd || (xml[pos] != '"' && xml[pos] != '\'')) {
            continue;
        }

        const char quote = xml[pos++];
        const size_t valueBegin = pos;
        while (pos < tagEnd && xml[pos] != quote) {
            ++pos;
        }
        const std::string attrValueUtf8 = htmlEntityDecode(xml.substr(valueBegin, pos - valueBegin));
        if (pos < tagEnd) {
            ++pos;
        }
        attrs[common::utf8ToWide(attrNameUtf8)] = common::utf8ToWide(attrValueUtf8);
    }
    return attrs;
}

std::vector<ConfigManager::UserAccountEntry> parseUserAccountsXml(const std::string& xml, int maxCount)
{
    std::vector<ConfigManager::UserAccountEntry> accounts;
    if (maxCount <= 0) {
        maxCount = 10;
    }

    const size_t accountsStart = findStartTag(xml, "Accounts");
    if (accountsStart == std::string::npos) {
        return accounts;
    }

    size_t accountsTagEnd = std::string::npos;
    readStartTagAttributes(xml, accountsStart, &accountsTagEnd);
    if (accountsTagEnd == std::string::npos) {
        return accounts;
    }
    if (accountsTagEnd > accountsStart && xml[accountsTagEnd - 1] == '/') {
        return accounts;
    }

    const size_t accountsEnd = xml.find("</Accounts>", accountsTagEnd);
    const size_t scanEnd = accountsEnd == std::string::npos ? xml.size() : accountsEnd;
    size_t pos = accountsTagEnd + 1;
    while (accounts.size() < static_cast<size_t>(maxCount)) {
        const size_t accountStart = xml.find("<Account", pos);
        if (accountStart == std::string::npos || accountStart >= scanEnd) {
            break;
        }
        const size_t next = accountStart + std::strlen("<Account");
        if (next < xml.size() && !isTagNameTerminator(xml[next])) {
            pos = next;
            continue;
        }

        size_t accountTagEnd = std::string::npos;
        const ParsedAttributeMap attrs = readStartTagAttributes(xml, accountStart, &accountTagEnd);
        if (accountTagEnd == std::string::npos || accountTagEnd > scanEnd) {
            break;
        }
        pos = accountTagEnd + 1;

        const auto nameIt = attrs.find(L"name");
        if (nameIt == attrs.end()) {
            continue;
        }
        ConfigManager::UserAccountEntry entry;
        entry.name = trimAccountName(nameIt->second);
        if (entry.name.empty()) {
            continue;
        }
        const auto duplicateIt = std::find_if(accounts.begin(), accounts.end(), [&entry](const ConfigManager::UserAccountEntry& item) {
            return equalsIgnoreCase(item.name, entry.name);
        });
        if (duplicateIt != accounts.end()) {
            continue;
        }

        const auto typeIt = attrs.find(L"type");
        if (typeIt != attrs.end()) {
            wchar_t* end = nullptr;
            const long parsed = std::wcstol(typeIt->second.c_str(), &end, 10);
            entry.type = end && *end == L'\0' ? static_cast<int>(parsed) : 0;
        }
        accounts.push_back(entry);
    }
    return accounts;
}

std::string serializedUserAccounts(const std::vector<ConfigManager::UserAccountEntry>& accounts)
{
    if (accounts.empty()) {
        return "<Accounts />";
    }

    std::string text = "<Accounts>\r\n";
    for (const auto& account : accounts) {
        text += "<Account name=\"" + htmlEntityEncode(common::wideToUtf8(account.name)) +
            "\" type=\"" + std::to_string(account.type) + "\" />\r\n";
    }
    text += "</Accounts>";
    return text;
}

std::string upsertUserAccountsXml(std::string xml, const std::vector<ConfigManager::UserAccountEntry>& accounts)
{
    if (xml.empty()) {
        xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<UserInfo>\r\n</UserInfo>\r\n";
    }

    const std::string accountsXml = serializedUserAccounts(accounts);
    const size_t accountsStart = findStartTag(xml, "Accounts");
    if (accountsStart != std::string::npos) {
        size_t tagEnd = std::string::npos;
        readStartTagAttributes(xml, accountsStart, &tagEnd);
        if (tagEnd == std::string::npos) {
            return xml;
        }

        size_t replaceEnd = tagEnd + 1;
        if (!(tagEnd > accountsStart && xml[tagEnd - 1] == '/')) {
            const size_t closing = xml.find("</Accounts>", tagEnd);
            if (closing != std::string::npos) {
                replaceEnd = closing + std::strlen("</Accounts>");
            }
        }
        xml.replace(accountsStart, replaceEnd - accountsStart, accountsXml);
        return xml;
    }

    const size_t userInfoClose = xml.rfind("</UserInfo>");
    if (userInfoClose != std::string::npos) {
        xml.insert(userInfoClose, "\r\n" + accountsXml + "\r\n");
        return xml;
    }

    return "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<UserInfo>\r\n" + accountsXml + "\r\n</UserInfo>\r\n";
}

std::string userInfoXmlTextForPath(const std::wstring& path)
{
    std::string xml;
    if (fileExists(path) && readFileBytes(path, &xml)) {
        return xml;
    }
    return kDefaultUserInfo;
}

}

bool ConfigManager::load(const std::wstring& sdologinDirectory)
{
    rootDirectory_ = sdologinDirectory;
    configs_.clear();

    bool loadedAny = false;
    loadedAny = loadOne(ConfigFile::ClientInfo, L"clientinfo") || loadedAny;
    loadedAny = loadOne(ConfigFile::System, L"config") || loadedAny;
    loadedAny = loadOne(ConfigFile::Version, L"version") || loadedAny;
    loadedAny = loadOne(ConfigFile::Browser, L"browser") || loadedAny;
    loadedAny = loadOne(ConfigFile::Widget, L"widget") || loadedAny;
    loadedAny = loadOne(ConfigFile::UserInfo, L"userinfo", kDefaultUserInfo) || loadedAny;
    loadedAny = loadOne(ConfigFile::SkinType, L"skintype") || loadedAny;
    return loadedAny;
}

std::wstring ConfigManager::value(ConfigFile file, const std::wstring& node, const std::wstring& attribute, const std::wstring& fallback) const
{
    const NodeMap* nodes = nodeMap(file);
    if (!nodes) {
        return fallback;
    }

    const auto nodeIt = nodes->find(node);
    if (nodeIt == nodes->end()) {
        return fallback;
    }

    const auto attrIt = nodeIt->second.find(attribute);
    if (attrIt == nodeIt->second.end()) {
        return fallback;
    }
    return attrIt->second;
}

bool ConfigManager::booleanValue(ConfigFile file, const std::wstring& node, bool fallback) const
{
    const std::wstring raw = value(file, node);
    if (raw.empty()) {
        return fallback;
    }
    if (raw == L"1" || equalsIgnoreCase(raw, L"true") || equalsIgnoreCase(raw, L"yes")) {
        return true;
    }
    if (raw == L"0" || equalsIgnoreCase(raw, L"false") || equalsIgnoreCase(raw, L"no")) {
        return false;
    }
    return fallback;
}

int ConfigManager::integerValue(ConfigFile file, const std::wstring& node, int fallback) const
{
    const std::wstring raw = value(file, node);
    if (raw.empty()) {
        return fallback;
    }
    wchar_t* end = nullptr;
    const long parsed = std::wcstol(raw.c_str(), &end, 10);
    return end && *end == L'\0' ? static_cast<int>(parsed) : fallback;
}

bool ConfigManager::setValue(ConfigFile file, const std::wstring& node, const std::wstring& newValue, const std::wstring& attribute)
{
    if (rootDirectory_.empty() || node.empty() || attribute.empty()) {
        return false;
    }

    const std::wstring path = writableConfigPath(rootDirectory_, file);
    std::string xml;
    if (fileExists(path) && !readFileBytes(path, &xml)) {
        return false;
    }

    const std::string updatedXml = upsertXmlAttribute(
        xml,
        rootNodeName(file),
        common::wideToUtf8(node),
        common::wideToUtf8(attribute),
        common::wideToUtf8(newValue));
    if (!writeFileBytes(path, updatedXml)) {
        return false;
    }

    configs_[file][node][attribute] = newValue;
    return true;
}

bool ConfigManager::setIntegerValue(ConfigFile file, const std::wstring& node, int newValue)
{
    return setValue(file, node, std::to_wstring(newValue));
}

std::wstring ConfigManager::clientInfoValue(const std::wstring& node, const std::wstring& fallback) const
{
    return value(ConfigFile::ClientInfo, node, L"value", fallback);
}

std::wstring ConfigManager::systemValue(const std::wstring& node, const std::wstring& fallback) const
{
    return value(ConfigFile::System, node, L"value", fallback);
}

bool ConfigManager::quickLoginEnabled() const
{
    if (!value(ConfigFile::UserInfo, L"EnableQuickLogin").empty()) {
        return booleanValue(ConfigFile::UserInfo, L"EnableQuickLogin", true);
    }
    if (!value(ConfigFile::Widget, L"EnableQuickLogin").empty()) {
        return booleanValue(ConfigFile::Widget, L"EnableQuickLogin", true);
    }
    return booleanValue(ConfigFile::ClientInfo, L"EnableQuickLogin", true);
}

int ConfigManager::maxAccountCount(int fallback) const
{
    if (fallback <= 0) {
        fallback = 10;
    }
    const int configured = integerValue(ConfigFile::System, L"MaxAccountCount", fallback);
    return configured > 0 ? configured : fallback;
}

std::vector<ConfigManager::UserAccountEntry> ConfigManager::userAccounts(int maxCount) const
{
    if (rootDirectory_.empty()) {
        return {};
    }

    const std::wstring path = writableConfigPath(rootDirectory_, ConfigFile::UserInfo);
    return parseUserAccountsXml(userInfoXmlTextForPath(path), maxCount);
}

bool ConfigManager::recordUserAccount(const std::wstring& account, int type, int maxCount)
{
    if (rootDirectory_.empty()) {
        return false;
    }
    if (maxCount <= 0) {
        maxCount = 10;
    }

    const std::wstring normalized = trimAccountName(account);
    if (normalized.empty()) {
        return false;
    }

    const std::wstring path = writableConfigPath(rootDirectory_, ConfigFile::UserInfo);
    std::string xml = userInfoXmlTextForPath(path);
    std::vector<UserAccountEntry> accounts = parseUserAccountsXml(xml, 1000);
    accounts.erase(std::remove_if(accounts.begin(), accounts.end(), [&normalized](const UserAccountEntry& item) {
        return equalsIgnoreCase(item.name, normalized);
    }), accounts.end());
    accounts.insert(accounts.begin(), UserAccountEntry{normalized, type});
    if (accounts.size() > static_cast<size_t>(maxCount)) {
        accounts.resize(static_cast<size_t>(maxCount));
    }

    const std::string updatedXml = upsertUserAccountsXml(xml, accounts);
    if (!writeFileBytes(path, updatedXml)) {
        return false;
    }
    parseXmlText(ConfigFile::UserInfo, updatedXml);
    return true;
}

bool ConfigManager::removeUserAccount(const std::wstring& account)
{
    if (rootDirectory_.empty()) {
        return false;
    }

    const std::wstring normalized = trimAccountName(account);
    if (normalized.empty()) {
        return false;
    }

    const std::wstring path = writableConfigPath(rootDirectory_, ConfigFile::UserInfo);
    std::string xml = userInfoXmlTextForPath(path);
    std::vector<UserAccountEntry> accounts = parseUserAccountsXml(xml, 1000);
    const size_t before = accounts.size();
    accounts.erase(std::remove_if(accounts.begin(), accounts.end(), [&normalized](const UserAccountEntry& item) {
        return equalsIgnoreCase(item.name, normalized);
    }), accounts.end());
    if (accounts.size() == before) {
        return false;
    }

    const std::string updatedXml = upsertUserAccountsXml(xml, accounts);
    if (!writeFileBytes(path, updatedXml)) {
        return false;
    }
    parseXmlText(ConfigFile::UserInfo, updatedXml);
    return true;
}

bool ConfigManager::loadOne(ConfigFile file, const std::wstring& fileStem, const char* defaultXml)
{
    const std::wstring configDir = common::joinPath(rootDirectory_, L"config");
    const std::wstring xmlPath = common::joinPath(configDir, fileStem + L".xml");
    const std::wstring cfgPath = common::joinPath(configDir, fileStem + L".cfg");
    const std::wstring rootXmlPath = common::joinPath(rootDirectory_, fileStem + L".xml");
    const std::wstring rootCfgPath = common::joinPath(rootDirectory_, fileStem + L".cfg");

    if (loadXmlText(file, xmlPath) || loadXmlText(file, cfgPath) ||
        loadXmlText(file, rootXmlPath) || loadXmlText(file, rootCfgPath)) {
        return true;
    }

    if (defaultXml) {
        parseXmlText(file, defaultXml);
        return true;
    }
    return false;
}

bool ConfigManager::loadXmlText(ConfigFile file, const std::wstring& fullPath)
{
    if (!fileExists(fullPath)) {
        return false;
    }
    std::string xml;
    if (!readFileBytes(fullPath, &xml)) {
        return false;
    }
    parseXmlText(file, xml);
    return true;
}

void ConfigManager::parseXmlText(ConfigFile file, const std::string& xmlText)
{
    NodeMap nodes;
    size_t pos = 0;
    while ((pos = xmlText.find('<', pos)) != std::string::npos) {
        ++pos;
        if (pos >= xmlText.size()) {
            break;
        }
        if (xmlText[pos] == '?' || xmlText[pos] == '/' || xmlText[pos] == '!') {
            continue;
        }

        const std::string nodeNameUtf8 = readName(xmlText, &pos);
        if (nodeNameUtf8.empty()) {
            continue;
        }

        AttributeMap attrs;
        while (pos < xmlText.size() && xmlText[pos] != '>') {
            if (xmlText[pos] == '/') {
                ++pos;
                continue;
            }
            skipSpaces(xmlText, &pos);
            if (pos >= xmlText.size() || xmlText[pos] == '>' || xmlText[pos] == '/') {
                continue;
            }

            const std::string attrNameUtf8 = readName(xmlText, &pos);
            skipSpaces(xmlText, &pos);
            if (attrNameUtf8.empty() || pos >= xmlText.size() || xmlText[pos] != '=') {
                continue;
            }
            ++pos;
            skipSpaces(xmlText, &pos);
            if (pos >= xmlText.size() || (xmlText[pos] != '"' && xmlText[pos] != '\'')) {
                continue;
            }

            const char quote = xmlText[pos++];
            const size_t valueBegin = pos;
            while (pos < xmlText.size() && xmlText[pos] != quote) {
                ++pos;
            }
            const std::string attrValueUtf8 = htmlEntityDecode(xmlText.substr(valueBegin, pos - valueBegin));
            if (pos < xmlText.size()) {
                ++pos;
            }
            attrs[common::utf8ToWide(attrNameUtf8)] = common::utf8ToWide(attrValueUtf8);
        }

        if (!attrs.empty()) {
            nodes[common::utf8ToWide(nodeNameUtf8)] = attrs;
        }
    }
    configs_[file] = nodes;
}

const ConfigManager::NodeMap* ConfigManager::nodeMap(ConfigFile file) const
{
    const auto it = configs_.find(file);
    return it == configs_.end() ? nullptr : &it->second;
}

}
