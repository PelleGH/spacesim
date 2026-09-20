#include "game/runtime/GameApplication.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        SpaceSim::GameApplication application;
        return application.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SpaceSim fatal error: " << exception.what() << '\n';
        return 1;
    }
}
