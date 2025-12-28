#pragma once
#include <functional>

/*---------
	Job
----------*/

using CallbackType = std::function<void()>;

class Job
{
public:
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
		_createdTime = ::GetTickCount64();
	}

	template<typename T, typename Ret, typename... Args>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
	{
		_callback = [owner, memFunc, args...]()
		{
			(owner.get()->*memFunc)(args...);
		};
		_createdTime = ::GetTickCount64();
	}

	void Execute()
	{
		_callback();
	}

	uint64 GetCreatedTime() const { return _createdTime; }

private:
	CallbackType _callback;
	uint64 _createdTime = 0;

};

