#include "pch.h"
#include "gtest/gtest.h"
#include "Job.h"



// 테스트용 더미 클래스
class TestObject : public std::enable_shared_from_this<TestObject>
{
public:
    int value = 0;

    void SetValue(int v) { value = v; }
    void Add(int a, int b) { value = a + b; }
    void Increment() { value++; }
};

/*---------------------------
    람다 콜백 테스트
----------------------------*/
TEST(JobTest, LambdaCallback_ExecutesCorrectly)
{
    // Arrange
    bool executed = false;
    Job job([&executed]() { executed = true; });

    // Act
    job.Execute();

    // Assert
    EXPECT_TRUE(executed);
}

TEST(JobTest, LambdaCallback_ModifiesValue)
{
    // Arrange
    int result = 0;
    Job job([&result]() { result = 42; });

    // Act
    job.Execute();

    // Assert
    EXPECT_EQ(result, 42);
}

/*---------------------------
    멤버 함수 포인터 테스트
----------------------------*/
TEST(JobTest, MemberFunction_NoArgs)
{
    // Arrange
    auto obj = std::make_shared<TestObject>();
    obj->value = 10;
    Job job(obj, &TestObject::Increment);

    // Act
    job.Execute();

    // Assert
    EXPECT_EQ(obj->value, 11);
}

TEST(JobTest, MemberFunction_SingleArg)
{
    // Arrange
    auto obj = std::make_shared<TestObject>();
    Job job(obj, &TestObject::SetValue, 100);

    // Act
    job.Execute();

    // Assert
    EXPECT_EQ(obj->value, 100);
}

TEST(JobTest, MemberFunction_MultipleArgs)
{
    // Arrange
    auto obj = std::make_shared<TestObject>();
    Job job(obj, &TestObject::Add, 30, 12);

    // Act
    job.Execute();

    // Assert
    EXPECT_EQ(obj->value, 42);
}

/*---------------------------
    생성 시간 테스트
----------------------------*/
TEST(JobTest, CreatedTime_IsSet)
{
    // Arrange & Act
    uint64 beforeTime = ::GetTickCount64();
    Job job([]() {});
    uint64 afterTime = ::GetTickCount64();

    // Assert
    EXPECT_GE(job.GetCreatedTime(), beforeTime);
    EXPECT_LE(job.GetCreatedTime(), afterTime);
}

TEST(JobTest, CreatedTime_DifferentJobs)
{
    // Arrange
    Job job1([]() {});
    Sleep(10);  // 시간 차이 생성
    Job job2([]() {});

    // Assert
    EXPECT_LE(job1.GetCreatedTime(), job2.GetCreatedTime());
}

/*---------------------------
    다중 실행 테스트
----------------------------*/
TEST(JobTest, Execute_MultipleTimes)
{
    // Arrange
    int counter = 0;
    Job job([&counter]() { counter++; });

    // Act
    job.Execute();
    job.Execute();
    job.Execute();

    // Assert
    EXPECT_EQ(counter, 3);
}

/*---------------------------
    shared_ptr 수명 테스트
----------------------------*/
TEST(JobTest, SharedPtr_KeepsObjectAlive)
{
    // Arrange
    std::weak_ptr<TestObject> weakPtr;
    Job* jobPtr = nullptr;

    {
        auto obj = std::make_shared<TestObject>();
        weakPtr = obj;
        jobPtr = new Job(obj, &TestObject::SetValue, 999);
    }
    // obj는 scope를 벗어났지만 Job이 shared_ptr을 들고 있음

    // Assert - 객체가 아직 살아있어야 함
    EXPECT_FALSE(weakPtr.expired());

    // Act
    jobPtr->Execute();
    EXPECT_EQ(weakPtr.lock()->value, 999);

    // Cleanup
    delete jobPtr;

    // 이제 객체가 해제되어야 함
    EXPECT_TRUE(weakPtr.expired());
}
