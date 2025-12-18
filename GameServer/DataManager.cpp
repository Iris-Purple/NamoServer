#include "pch.h"
#include "DataManager.h"

void DataManager::Init(const string& dataPath)
{
	_dataPath = dataPath;

	Data::StatData statData = LoadJson<Data::StatData>("StatData.json");
	_statDict = statData.MakeDict();
	cout << "[DataManager] StatData.json Loaded " << _statDict.size() << " stats" << endl;

	Data::SkillData skillData = LoadJson<Data::SkillData>("SkillData.json");
	_skillDict = skillData.MakeDict();
	cout << "[DataManager] SkillData.json Loaded " << _skillDict.size() << " skills" << endl;
}

const Data::Stat* DataManager::GetStat(int level) const
{
	auto it = _statDict.find(level);
	if (it != _statDict.end())
		return &it->second;

	return nullptr;
}
const Data::Skill* DataManager::GetSkill(int id) const
{
	auto it = _skillDict.find(id);
	if (it != _skillDict.end())
		return &it->second;
	return nullptr;
}

template<typename TLoader>
TLoader DataManager::LoadJson(const string& path)
{
	string fullPath = _dataPath + path;
	ifstream file(fullPath);

	if (!file.is_open())
	{
		cerr << "[DataManager] Failed to open: " << fullPath << endl;
		return TLoader{};
	}
	json j;
	file >> j;
	return j.get<TLoader>();
}

template Data::StatData DataManager::LoadJson<Data::StatData>(const string& path);
template Data::SkillData DataManager::LoadJson<Data::SkillData>(const string& path);


