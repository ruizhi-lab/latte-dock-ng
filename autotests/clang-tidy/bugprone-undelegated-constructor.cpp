struct ConstructorDelegation
{
    ConstructorDelegation(int) {}
    ConstructorDelegation()
    {
        // Keep a same-class temporary instead of delegating to the other constructor.
        ConstructorDelegation(1);
    }
};
