void
enclosingFunction()
{
    auto lambda = [] { (void)__func__; };
    lambda();
}
