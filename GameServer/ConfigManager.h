#pragma once

#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

	struct ServerConfig
	{
		string dataPath;
		bool encryptionEnabled = false;  // 패킷 암호화 ON/OFF
		string encryptionKey;            // AES-128 키 (16바이트 문자열)
		int roomCount;					 // 방 갯수
		int monsterCount;				 // 몬스터 갯수
	};

	inline void from_json(const json& j, ServerConfig& config)
	{
		j.at("dataPath").get_to(config.dataPath);

		// encryptionEnabled는 선택적 (없으면 기본값 false)
		if (j.contains("encryptionEnabled"))
			j.at("encryptionEnabled").get_to(config.encryptionEnabled);

		// encryptionKey는 선택적 (없으면 CoreGlobal 기본값 사용)
		if (j.contains("encryptionKey"))
			j.at("encryptionKey").get_to(config.encryptionKey);

		if (j.contains("roomCount"))
			j.at("roomCount").get_to(config.roomCount);
		if (j.contains("monsterCount"))
			j.at("monsterCount").get_to(config.monsterCount);

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
		const string& GetEncryptionKey() const { return _config.encryptionKey; }
		const int& GetRoomCount() const { return _config.roomCount; }
		const int& GetMonsterCount() const { return _config.monsterCount; }

	private:
		ConfigManager() = default;
		~ConfigManager() = default;
		ConfigManager(const ConfigManager&) = delete;
		ConfigManager& operator=(const ConfigManager&) = delete;

	private:
		ServerConfig _config;
	};
