#include "pch.h"
#include "ConfigManager.h"


    void ConfigManager::LoadConfig(const std::string& path)
    {
        ifstream file(path);

        if (!file.is_open())
        {
            cerr << "[ConfigManager] Failed to open: " << path << endl;
            return;
        }

        json j;
        file >> j;

        _config = j.get<ServerConfig>();

        // 전역 암호화 설정 적용
        GEncryptionEnabled = _config.encryptionEnabled;

        // 암호화 키 적용 (16바이트 문자열 → GEncryptionKey)
        if (!_config.encryptionKey.empty() && _config.encryptionKey.length() == 16)
        {
            memcpy(GEncryptionKey, _config.encryptionKey.c_str(), 16);
        }

        cout << "[ConfigManager] Config loaded" << endl;
        cout << "  - dataPath: " << _config.dataPath << endl;
        cout << "  - encryptionEnabled: " << (_config.encryptionEnabled ? "true" : "false") << endl;
        cout << "  - encryptionKey: " << (_config.encryptionKey.empty() ? "(default)" : "(loaded)") << endl;
    }
