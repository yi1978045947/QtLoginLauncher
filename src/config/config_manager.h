#pragma once

#include <map>
#include <string>
#include <vector>

namespace qtlogin::config {

enum class ConfigFile {
    ClientInfo,
    System,
    Version,
    UserInfo,
    Browser,
    Widget,
    SkinType
};

class ConfigManager {
public:
    struct UserAccountEntry {
        std::wstring name;
        int type = 0;
    };

    bool load(const std::wstring& sdologinDirectory);

    std::wstring value(
        ConfigFile file,
        const std::wstring& node,
        const std::wstring& attribute = L"value",
        const std::wstring& fallback = L"") const;

    bool booleanValue(ConfigFile file, const std::wstring& node, bool fallback) const;
    int integerValue(ConfigFile file, const std::wstring& node, int fallback) const;
    bool setValue(
        ConfigFile file,
        const std::wstring& node,
        const std::wstring& newValue,
        const std::wstring& attribute = L"value");
    bool setIntegerValue(ConfigFile file, const std::wstring& node, int newValue);

    std::wstring clientInfoValue(const std::wstring& node, const std::wstring& fallback = L"") const;
    std::wstring systemValue(const std::wstring& node, const std::wstring& fallback = L"") const;
    bool quickLoginEnabled() const;
    int maxAccountCount(int fallback) const;
    std::vector<UserAccountEntry> userAccounts(int maxCount) const;
    bool recordUserAccount(const std::wstring& account, int type, int maxCount);
    bool removeUserAccount(const std::wstring& account);

    const std::wstring& rootDirectory() const { return rootDirectory_; }

private:
    using AttributeMap = std::map<std::wstring, std::wstring>;
    using NodeMap = std::map<std::wstring, AttributeMap>;

    bool loadOne(ConfigFile file, const std::wstring& fileStem, const char* defaultXml = nullptr);
    bool loadXmlText(ConfigFile file, const std::wstring& fullPath);
    void parseXmlText(ConfigFile file, const std::string& xmlText);

    const NodeMap* nodeMap(ConfigFile file) const;

    std::wstring rootDirectory_;
    std::map<ConfigFile, NodeMap> configs_;
};

}
