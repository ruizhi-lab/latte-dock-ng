// Negative fixture for scripts/test-clazy.py: must trigger the clazy rule-of-two-soft check.
class Rule {
public:
    Rule() = default;
    Rule &operator=(const Rule &other) { value = other.value + 1; return *this; }
    int value = 0;
};
Rule probe(const Rule &source)
{
    Rule copy(source);
    return copy;
}
