#include "timeit.h"

Timeit::Timeit(std::string description)
: _description(description)
, _running(true)
{
    _start = std::chrono::high_resolution_clock::now();
}

Timeit::~Timeit()
{
    stop();
    print();
}

void Timeit::stop()
{
    _stop = std::chrono::high_resolution_clock::now();
    _running = false;
}

void Timeit::print()
{
    if (_running)
    {
        stop();
        std::chrono::duration<float> diff = _stop - _start;

        printf("%s took %f ms\n", _description.c_str(), diff.count() * 1000);
    }
}
