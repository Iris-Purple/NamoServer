#pragma once

class ObjectManager
{
public:
	static ObjectManager& Instance()
	{
		static ObjectManager instance;
		return instance;
	}

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;
	bool Remove(int32 objectId);
	PlayerRef Find(int32 objectId);

private:
	ObjectManager() = default;
	~ObjectManager() = default;

	USE_LOCK;
	unordered_map<int32, PlayerRef> _players;
	int32 _counter = 0;

	// [UNUSED(1)][TYPE(7)][ID(24)]
	int32 GenerateId(Protocol::GameObjectType type)
	{
		// 이미 lock이 잡힌 상태에서 호출됨
		return (static_cast<int32>(type) << 24) | (_counter++);
	}

public:
	template <typename T>
	shared_ptr<T> Add()
	{
		// T가 GameObject를 상속하는지 컴파일 타임 체크
		static_assert(std::is_base_of_v<GameObject, T>,
			"T must inherit from GameObject");

		auto gameObject = std::make_shared<T>();

		WRITE_LOCK;
		gameObject->SetId(GenerateId(gameObject->GetObjectType()));
		if (gameObject->GetObjectType() == Protocol::GameObjectType::PLAYER) 
		{
			_players[gameObject->Id()] = std::dynamic_pointer_cast<Player>(gameObject);
		}

		return gameObject;
	}

	// ID로 오브젝트 타입 추출
	static Protocol::GameObjectType GetObjectTypeById(int32 id)
	{
		int32 type = (id >> 24) & 0x7F;
		return static_cast<Protocol::GameObjectType>(type);
	}

	
};

