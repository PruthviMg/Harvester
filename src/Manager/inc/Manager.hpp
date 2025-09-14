#ifndef MANAGER_HPP
#define MANAGER_HPP

#include "Game.hpp"
#include "Platform.hpp"
#include "SoilTesting.hpp"
#include "Plough.hpp"
#include <memory>

namespace Harvestor
{
    class Manager
    {
    public:
        Manager();

        ~Manager();

        void initialize();

        void deInitialize();

        void run();

    private:
        std::unique_ptr<Platform> platform_;
        SoilTesting soilTesting_;
        Plough plough_;
    };
}

#endif