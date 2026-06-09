#include "core/Application.h"

#include <exception>
#include <iostream>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    try
    {
        SpaceSim::Application app;
        return app.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << "\n";
        return 1;
    }
}