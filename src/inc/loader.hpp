#ifndef LOADER_HPP_
#define LOADER_HPP_

#include "QualityMatrix.hpp"
#include "common.hpp"
#include "land.hpp"

namespace Harvestor {
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class SoilLoader {
   public:
    // Load soil data from a CSV or whitespace-separated file
    // Each line = 7 floats: soilBaseQuality sunlight nutrients pH organicMatter compaction salinity
    static std::vector<std::array<float, 9>> loadFromFile(const std::string &filename) {
        std::vector<std::array<float, 9>> soilMatrix;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open soil file: " << filename << "\n";
            return soilMatrix;
        }

        std::string line;
        bool isHeader = true;

        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) { return !std::isspace(ch); }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](int ch) { return !std::isspace(ch); }).base(), line.end());

            if (line.empty() || line[0] == '#') continue;

            // Skip header row if present
            if (isHeader) {
                isHeader = false;
                if (line.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos) {
                    continue;  // assume header contains letters
                }
            }

            std::stringstream ss(line);
            std::string token;

            // First two columns are x, y (skip)
            // if (!std::getline(ss, token, ',')) continue;  // x
            // if (!std::getline(ss, token, ',')) continue;  // y

            std::array<float, 9> vals;
            bool valid = true;

            for (int i = 0; i < 9; ++i) {
                if (!std::getline(ss, token, ',')) {
                    valid = false;
                    break;
                }

                // Remove potential whitespace
                token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](int ch) { return !std::isspace(ch); }));
                token.erase(std::find_if(token.rbegin(), token.rend(), [](int ch) { return !std::isspace(ch); }).base(), token.end());

                try {
                    vals[i] = std::stof(token);
                } catch (const std::exception &e) {
                    std::cerr << "Invalid float in soil data: '" << token << "' in line: " << line << "\n";
                    valid = false;
                    break;
                }
            }

            if (valid) {
                soilMatrix.push_back(vals);
            } else {
                std::cerr << "Skipping invalid soil line: " << line << "\n";
            }
        }

        return soilMatrix;
    }
};

class FarmLoader {
   public:
    static std::vector<sf::Vector2f> parseCSV(const std::string &filename) {
        std::vector<sf::Vector2f> result;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open CSV file: " << filename << std::endl;
            return result;
        }

        std::string line;
        // Skip header
        if (!std::getline(file, line)) return result;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string xStr, yStr;
            if (!std::getline(ss, xStr, ',')) continue;
            if (!std::getline(ss, yStr, ',')) continue;

            try {
                float x = std::stof(xStr);
                float y = std::stof(yStr);
                result.emplace_back(x, y);
            } catch (...) {
                // skip invalid lines
                continue;
            }
        }

        return result;
    }

    static void loadFromFile(const std::string &filename, std::vector<Land> &lands, std::vector<Pond> &ponds, const std::vector<CropType> &crops,
                             int selectedCropIndex) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << "\n";
            return;
        }

        lands.clear();
        ponds.clear();

        auto soilMatrix = SoilLoader::loadFromFile("input/land.csv");
        Land land(Config::landTileSize);
        land.generateTiles(soilMatrix);

        lands.push_back(land);

        for (auto &land : lands) {
            if (selectedCropIndex >= 0 && selectedCropIndex < crops.size()) {
                land.plantCrops(crops[selectedCropIndex]);
            }
        }

        Pond pond(Config::landTileSize);
        auto points = parseCSV("input/water.csv");
        std::cout << "points " << points.size() << std::endl;
        pond.generate(points);

        ponds.emplace_back(pond);

        // struct TempLandData {
        //     float cx, cy, r;
        // };
        // std::vector<TempLandData> tempLands;

        // std::string type;
        // while (file >> type) {
        //     if (type[0] == '#') {
        //         std::getline(file, type);
        //         continue;
        //     }

        //     if (type == "Land") {
        //         float cx, cy, r;
        //         file >> cx >> cy >> r;
        //         tempLands.push_back({cx, cy, r});
        //     } else if (type == "Pond") {
        //         float cx, cy, r;
        //         file >> cx >> cy >> r;
        //         ponds.emplace_back(cx, cy, r, 0.9f, 0.9f);
        //     }
        // }

        // // ---------------- Generate combined soil matrix ----------------
        // std::vector<Tile> allTiles;

        // for (auto &l : tempLands) {
        //     // QualityMatrix matrix(l.cx, l.cy, l.r, Config::landTileSize);
        //     auto soilMatrix = SoilLoader::loadFromFile("input/soil_data.csv");
        //     // auto tiles = matrix.generateTiles();

        //     Land land(Config::landTileSize);
        //     land.generateTiles(soilMatrix);

        //     if (selectedCropIndex >= 0 && selectedCropIndex < crops.size()) {
        //         land.plantCrops(crops[selectedCropIndex]);
        //     }

        //     lands.push_back(land);

        //     allTiles.insert(allTiles.end(), tiles.begin(), tiles.end());
        // }

        // // Save the combined soil data to CSV
        // std::ofstream soilFile(Config::soilDataFile);
        // if (!soilFile.is_open()) {
        //     std::cerr << "Error: Could not open soil_data.csv for writing\n";
        // } else {
        //     soilFile << "x,y,soilBaseQuality,sunlight,nutrients,pH,organicMatter,compaction,salinity\n";
        //     for (auto &tile : allTiles) {
        //         soilFile << tile.position.x << "," << tile.position.y << "," << tile.soilBaseQuality << "," << tile.sunlight << "," <<
        //         tile.nutrients
        //                  << "," << tile.pH << "," << tile.organicMatter << "," << tile.compaction << "," << tile.salinity << "\n";
        //     }
        // }
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