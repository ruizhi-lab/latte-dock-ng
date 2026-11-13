class Base
{
  public:
    virtual ~Base() = default;
    virtual void resizeEvent() {}
};

class Intermediate : public Base
{
  public:
    ~Intermediate() override = default;
    void resizeEvent() override {}
};

class Derived : public Intermediate
{
  public:
    ~Derived() override = default;
    void resizeEvent() override { Base::resizeEvent(); }
};
