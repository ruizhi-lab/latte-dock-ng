struct Base
{
    virtual ~Base() = default;
};

struct Derived : Base
{};

Derived *
castThroughVoid(Base *base)
{
    // Keep the intermediate void pointer to trigger bugprone-casting-through-void.
    return static_cast<Derived *>(static_cast<void *>(base));
}
