struct Base
{
    virtual void run();
};

struct Derived : Base
{
    void run() override;
};

bool
sameVirtualMember()
{
    // Comparing two virtual member pointers triggers this check's unspecified behavior warning.
    return &Base::run == &Derived::run;
}
