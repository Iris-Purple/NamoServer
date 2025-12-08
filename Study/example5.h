#pragma once
class example5
{
public:
	static void Do();
};



class A {
public:
    std::mutex m;
    void CallB(class B& b);
};

class B {
public:
    std::mutex m;
    void CallA(A& a);
    
};

