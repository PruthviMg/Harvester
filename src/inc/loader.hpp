#ifndef LOADER_HPP_
#define LOADER_HPP_

#include "common.hpp"
#include "land.hpp"

namespace Harvestor {
class SoilLoader {
   public:
    // Load soil data from a CSV or simple space-separated file
    // Each line = 7 floats: soilBaseQuality sunlight nutrients pH organicMatter compaction salinity
    static std::vector<std::array<float, 7>> loadFromFile(const std::string &filename) {
        std::vector<std::array<float, 7>> soilMatrix;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open soil file: " << filename << "\n";
            return soilMatrix;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;  // skip comments
            std::stringstream ss(line);
            std::array<float, 7> vals;
            for (int i = 0; i < 7; ++i) {
                if (!(ss >> vals[i])) {
                    std::cerr << "Invalid soil data in line: " << line << "\n";
                    break;
                }
            }
            soilMatrix.push_back(vals);
        }

        return soilMatrix;
    }
};

class FarmLoader {
   public:
    static void loadFromFile(const std::string &filename, std::vector<Land> &lands, std::vector<Pond> &ponds, const std::vector<CropType> &crops,
                             int selectedCropIndex) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << "\n";
            return;
        }

        lands.clear();
        ponds.clear();

        std::string type;
        while (file >> type) {
            if (type[0] == '#') {
                std::getline(file, type);
                continue;
            }
            if (type == "Land") {
                float cx, cy, r;
                file >> cx >> cy >> r;

                Land land(cx, cy, r, Config::landTileSize);

                // Optionally, fetch soil values from soil loader here
                std::vector<std::array<float, 7>> soilMatrix = SoilLoader::loadFromFile(Config::soilDataFile);

                // Generate tiles using the updated Land class
                land.generateTiles(soilMatrix);

                // Plant crops on this land
                if (selectedCropIndex >= 0 && selectedCropIndex < crops.size()) {
                    land.plantCrops(crops[selectedCropIndex]);
                }

                lands.push_back(land);
            } else if (type == "Pond") {
                float cx, cy, r;
                file >> cx >> cy >> r;
                ponds.emplace_back(cx, cy, r, 0.9f, 0.9f);
            }
        }
    }
};

class CropLoader {
   public:
    static std::vector<CropType> loadFromFile(const std::string &filename) {
        std::vector<CropType> crops;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open crop file: " << filename << "\n";
            return crops;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;  // skip comments
            std::istringstream iss(line);
            CropType crop;
            int r, g, b;
            if (iss >> crop.name >> crop.baseGrowthRate >> r >> g >> b >> crop.optimalWater >> crop.tolerance) {
                crop.baseColor = sf::Color(r, g, b);
                crops.push_back(crop);
            } else {
                std::cerr << "Invalid crop line: " << line << "\n";
            }
        }
        return crops;
    }
};
}  // namespace Harvestor

#endif