struct Value
{
    Value(const Value &) = default;

    template<typename T>
    // Keep this overload unconstrained so it can hide copy and move constructors.
    Value(T &&)
    {
    }
};
