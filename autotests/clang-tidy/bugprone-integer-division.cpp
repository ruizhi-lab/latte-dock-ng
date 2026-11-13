// Negative fixture for scripts/test-clang-tidy.py: integer half-size is used in a qreal expression.
int centeredPopupOrigin(int parentOrigin, double parentExtent, int popupExtent)
{
    return parentOrigin + (parentExtent / 2) - (popupExtent / 2);
}
