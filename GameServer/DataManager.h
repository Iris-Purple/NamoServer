#pragma once

#include "pch.h"
#include <fstream>
#include "Stat.h"

class DataManager
{
public:
	static DataManager& Instance()
	{
		static DataManager instance;
		return instance;
	}
	void Init(const string& dataPath = "../Common/Data/");

	// Stat 접근
	const unordered_map<int, Data::Stat>& GetStatDict() const { return _statDict; }
	const Data::Stat* GetStat(int level) const;

	// Skill 접근
	const unordered_map<int, Data::Skill>& GetSkillDict() const { return _skillDict; }
	const Data::Skill* GetSkill(int id) const;

private:
	DataManager() = default;
	~DataManager() = default;
	DataManager(const DataManager&) = delete;
	DataManager& operator=(const DataManager&) = delete;

	template<typename TLoader>
	TLoader LoadJson(const std::string& path);

private:
	std::unordered_map<int, Data::Stat> _statDict;
	std::unordered_map<int, Data::Skill> _skillDict;
	std::string _dataPath;
};

