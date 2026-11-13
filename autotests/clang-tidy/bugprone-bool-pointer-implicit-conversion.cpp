void
boolPointerCondition(bool *value)
{
    // Keep the pointer condition implicit so the check catches pointer-to-bool conversion.
    if (value) {}
}
