#include <string>

class MoveConstructorCopiesString
{
  public:
    MoveConstructorCopiesString(MoveConstructorCopiesString &&other)
      : value(other.value)
    {
    }

  private:
    std::string value;
};
