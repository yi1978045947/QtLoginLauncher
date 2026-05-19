#pragma once
#include <string>
#include <vector>

class CurlHttpDownload
{
public:
    static bool DownloadToMemory(
        const std::string& url,
        std::vector<unsigned char>& outData,
        long timeoutMs = 5000,
        bool verifyCert = true   // 保留证书调试开关
    );
};
