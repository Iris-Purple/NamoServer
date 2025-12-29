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

        cout << "[ConfigManager] Config loaded" << endl;
        cout << "  - dataPath: " << _config.dataPath << endl;
        cout << "  - encryptionEnabled: " << (_config.encryptionEnabled ? "true" : "false") << endl;
    }
