#include "Engine.hpp"

#include <iostream>


int main()
{
    try
    {
        Engine engine;
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