#pragma once

#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

	struct ServerConfig
	{
		string dataPath;
		bool encryptionEnabled = false;  // 패킷 암호화 ON/OFF
	};

	inline void from_json(const json& j, ServerConfig& config)
	{
		j.at("dataPath").get_to(config.dataPath);

		// encryptionEnabled는 선택적 (없으면 기본값 false)
		if (j.contains("encryptionEnabled"))
			j.at("encryptionEnabled").get_to(config.encryptionEnabled);
	} 

	class ConfigManager
	{
	public:
		static ConfigManager& Instance()
		{
			static ConfigManager instance;
			return instance;
		}

		void LoadConfig(const string& path = "config.json");
		const string& GetDataPath() const { return _config.dataPath; }
		bool GetEncryptionEnabled() const { return _config.encryptionEnabled; }

	private:
		ConfigManager() = default;
		~ConfigManager() = default;
		ConfigManager(const ConfigManager&) = delete;
		ConfigManager& operator=(const ConfigManager&) = delete;

	private:
		ServerConfig _config;
	};
