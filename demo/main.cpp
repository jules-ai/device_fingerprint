#include <iostream>
#include <chrono>
#include "fingerprint.h"

int main(int argc, char **argv)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::string fingerprint = get_machine_fingerprint();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Time taken to get machine fingerprint: " << duration.count() / double(1000) << " ms" << std::endl;
    std::cout << "Machine Fingerprint: " << fingerprint << std::endl;
    return 0;
}
