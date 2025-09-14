#include "Manager.hpp"

int main()
{
    Harvestor::Manager manager;
    manager.initialize();
    manager.run();
    manager.deInitialize();

    return 0;
}
