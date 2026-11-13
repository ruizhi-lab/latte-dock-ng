void
checkTwice(bool ready)
{
    if (ready) {
        // Keep the unchanged nested condition to trigger the redundant-branch check.
        if (ready) {}
    }
}
