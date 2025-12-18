#pragma once

#include "pch.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"


using json = nlohmann::json;

namespace Data
{
	struct Stat
	{
		int level;
		int maxHp;
		int attack;
		int totalExp;
	};

	inline void from_json(const json& j, Stat& stat)
	{
		j.at("level").get_to(stat.level);
		j.at("maxHp").get_to(stat.maxHp);
		j.at("attack").get_to(stat.attack);
		j.at("totalExp").get_to(stat.totalExp);
	}

	struct StatData
	{
		vector<Stat> stats;
		unordered_map<int, Stat> MakeDict() const
		{
			unordered_map<int, Stat> dict;
			for (const Stat& stat : stats)
			{
				dict[stat.level] = stat;
			}
			return dict;
		}
	};

	inline void from_json(const json& j, StatData& data)
	{
		j.at("stats").get_to(data.stats);
	}

	

	struct ProjectileInfo
	{
		string name;
		float speed;
		int32 range;
		string prefab;
	};
	inline void from_json(const json& j, ProjectileInfo& info)
	{
		j.at("name").get_to(info.name);
		j.at("speed").get_to(info.speed);
		j.at("range").get_to(info.range);
		j.at("prefab").get_to(info.prefab);
	}

	struct Skill
	{
		int32 id;
		string name;
		float cooldown;
		int32 damage;
		Protocol::SkillType skillType;
		std::optional<ProjectileInfo> projectile;  // 없을 수도 있음
	};

	inline void from_json(const json& j, Skill& skill)
	{
		j.at("id").get_to(skill.id);
		j.at("name").get_to(skill.name);
		j.at("cooldown").get_to(skill.cooldown);
		j.at("damage").get_to(skill.damage);
		skill.skillType = static_cast<Protocol::SkillType>(j.at("skillType").get<int>());

		// projectile은 선택적 필드
		if (j.contains("projectile") && !j["projectile"].is_null())
		{
			skill.projectile = j["projectile"].get<ProjectileInfo>();
		}
	}

	struct SkillData
	{
		std::vector<Skill> skills;

		std::unordered_map<int, Skill> MakeDict() const
		{
			std::unordered_map<int, Skill> dict;
			for (const Skill& skill : skills)
			{
				dict[skill.id] = skill;
			}
			return dict;
		}
	};

	inline void from_json(const json& j, SkillData& data)
	{
		j.at("skills").get_to(data.skills);
	}
}



