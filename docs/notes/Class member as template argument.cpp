#include <iostream>
#include <array>

template <typename Derived>
class CRTP
{
    Derived &THIS;
  public:
    CRTP() : THIS(static_cast<Derived&>(*this)) {}
    void doProcess() {}
    void process()
    {
        THIS.doProcess();
    }
};

int idx = 0;

class TestComponent : public CRTP<TestComponent>
{
    public:
    int flag {0};
    int s{1};
    
    void doProcess()
    {
        flag = ++idx;
        std::cout << flag << std::endl;
    }
};

template <typename InternalComponent, auto FlagPtr>
class ArrayComponent : public CRTP<ArrayComponent<InternalComponent, FlagPtr>>
{
    std::array<InternalComponent, 5> components;
  public:
    void doProcess()
    {
        int sum = 0;
        for (auto &c: components)
        {
            c.process();
            sum += c.*FlagPtr;
        }
        std::cout << sum << std::endl;
    }
};

int main() {
    // Write C++ code here
    ArrayComponent<TestComponent, &TestComponent::flag> theTest;
    
    theTest.process();

    return 0;
}