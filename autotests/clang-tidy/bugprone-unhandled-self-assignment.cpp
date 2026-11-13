struct SelfAssign
{
    int *value;
    SelfAssign &operator=(const SelfAssign &other)
    {
        // Releasing before copying makes self-assignment lose the pointed value.
        delete value;
        value = new int(*other.value);
        return *this;
    }
};
