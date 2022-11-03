#ifndef _TIMEIT_H_
#define _TIMEIT_H_

#include <chrono>
#include <string>

class Timeit
{
public:
    Timeit(std::string description);
    ~Timeit();

    void stop();
    void print();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
    std::chrono::time_point<std::chrono::high_resolution_clock> _stop;
    std::string _description;
};

#endif // _TIMEIT_H_
