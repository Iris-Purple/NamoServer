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

        cout << "[ConfigManager] Config loaded" << endl;
        cout << "  - dataPath: " << _config.dataPath << endl;
    }
