struct CopyableBase
{
    CopyableBase() = default;
    CopyableBase(const CopyableBase &) = default;

    int value = 0;
};

struct DerivedWithoutBaseCopy : CopyableBase
{
    DerivedWithoutBaseCopy() = default;
    DerivedWithoutBaseCopy(const DerivedWithoutBaseCopy &other)
    {
        // Omit CopyableBase(other) so the check catches the lost base state.
    }
};
