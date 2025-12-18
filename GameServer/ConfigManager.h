#pragma once

#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

	struct ServerConfig
	{
		string dataPath;
	};

	inline void from_json(const json& j, ServerConfig& config)
	{
		j.at("dataPath").get_to(config.dataPath);
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

	private:
		ConfigManager() = default;
		~ConfigManager() = default;
		ConfigManager(const ConfigManager&) = delete;
		ConfigManager& operator=(const ConfigManager&) = delete;

	private:
		ServerConfig _config;
	};
