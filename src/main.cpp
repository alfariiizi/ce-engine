#include "Engine.hpp"
#include "SimpleBenchmark.hpp"

#include <iostream>


int main()
{
    try
    {
        Engine engine;

        // Start Computing
        SimpleBenchmark benchmark;  // Autocalculating if out-of-scope (in destructor)
        engine.Compute();
    }
    catch( const vk::SystemError& err )
    {
        std::cerr << err.what() << '\n';
        return EXIT_FAILURE;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Running Successfully\n";
    return EXIT_SUCCESS;
}