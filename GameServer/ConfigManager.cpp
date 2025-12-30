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
        else if (!_config.encryptionKey.empty())
        {
            cerr << "[ConfigManager] WARNING: encryptionKey must be exactly 16 bytes! "
                 << "(got " << _config.encryptionKey.length() << " bytes, using default key)" << endl;
        }

        cout << "[ConfigManager] Config loaded" << endl;
        cout << "  - dataPath: " << _config.dataPath << endl;
        cout << "  - encryptionEnabled: " << (_config.encryptionEnabled ? "true" : "false") << endl;

        if (_config.encryptionKey.empty())
            cout << "  - encryptionKey: (default)" << endl;
        else if (_config.encryptionKey.length() == 16)
            cout << "  - encryptionKey: (loaded from config)" << endl;
        else
            cout << "  - encryptionKey: (invalid length, using default)" << endl;
    }
