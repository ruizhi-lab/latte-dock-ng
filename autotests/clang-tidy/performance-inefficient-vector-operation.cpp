#include <vector>

void
inefficientVectorOperation(int count)
{
    std::vector<int> values;
    for (int index = 0; index < count; ++index) {
        values.push_back(index);
    }
}
