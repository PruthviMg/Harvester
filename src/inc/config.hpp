#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace Harvestor {
// ---------------- Config ----------------
struct Config {
    static inline int numLands = 3;
    static inline int numPonds = 3;
    static inline std::vector<float> landSizes = {120.f, 100.f, 140.f};
    static inline int rows = 20;
    static inline int cols = 30;
    static inline float landTileSize = 6.f;
    static inline float grassTileSize = 30.f;
    static inline float growthSpeed = 0.15f;
    static inline float uiPanelWidth = 150.f;

    // ---------------- Buttons ----------------
    static inline float cropButtonWidth = 130.f;
    static inline float cropButtonHeight = 30.f;
    static inline float cropButtonSpacing = 5.f;
    static inline float bigButtonHeightMultiplier = 3.f;  // big buttons (simulate/reset/rain)
    static inline float uiPadding = 10.f;
    static inline float ButtonPadding = 500.f;

    static inline unsigned int cropFontSize = 16;
    static inline unsigned int bigButtonFontSize = 18;

    static inline sf::Color cropButtonSelectedColor = sf::Color(100, 200, 250);
    static inline sf::Color cropButtonColor = sf::Color(200, 200, 200);
    static inline sf::Color simulateButtonColor = sf::Color(200, 100, 100);
    static inline sf::Color simulateButtonActiveColor = sf::Color(100, 200, 100);
    static inline sf::Color resetButtonColor = sf::Color(200, 200, 100);
    static inline sf::Color rainButtonColor = sf::Color(100, 100, 250);

    static inline float cropInitialScale = 0.3f;
    static inline float cropMaxScale = 2.2f;
    static inline float rainGrowthBoost = 0.2f;  // extra growth per second during rain
    static inline float rainDuration = 10.f;     // seconds
    static inline float rainIntensity = 0.5f;    // adjust value as needed

    static inline std::string fontPath = "resources/DejaVuSans.ttf";
    static inline std::string layoutFile = "input/farm_layout.txt";
    static inline std::string cropsFile = "input/crops.txt";
    static inline std::string soilDataFile = "input/soil_data.txt";

    // crops
    static inline int maxVisibleCrops = 4;
    static inline int maxVisibleLayouts = 4;

    // Raindrops
    static inline int numRaindrops = 2000;

    static inline std::mt19937 rng{12345};
};
}  // namespace Harvestor

#endif