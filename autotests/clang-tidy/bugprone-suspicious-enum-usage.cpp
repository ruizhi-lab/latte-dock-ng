enum FirstKind
{
    FirstZero = 0,
    FirstOne = 1,
    FirstTwo = 2
};
enum SecondKind
{
    SecondZero = 0,
    SecondOne = 1,
    SecondTwo = 2
};

unsigned int
overlapDifferentEnumRanges()
{
    // The overlapping numeric ranges make combining these unrelated enums suspicious.
    return FirstTwo | SecondOne;
}
