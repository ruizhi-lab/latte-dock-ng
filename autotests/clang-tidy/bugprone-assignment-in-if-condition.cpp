int
assignedInCondition(int value)
{
    // Keep the assignment in the condition to trigger bugprone-assignment-in-if-condition.
    if (value = 1) {
        return value;
    }
    return 0;
}
