#ifndef EVALUATOR_HPP_
#define EVALUATOR_HPP_

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"

namespace Harvestor {

class Evaluator {
   public:
    Evaluator(float precision = 0.001f) : precision_(precision) {}

    // Hash two floats with a given precision to avoid floating-point issues
    std::size_t hashTwoFloats(float a, float b) const {
        std::size_t h1 = std::hash<int>{}(static_cast<int>(a / precision_));
        std::size_t h2 = std::hash<int>{}(static_cast<int>(b / precision_));
        return h1 ^ (h2 << 1);
    }

    // Load tile/soil data
    bool updateSoilData(const std::string& filename = "soil_data.csv") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << filename << ": " << strerror(errno) << "\n";
            return false;
        }

        tile_map_.clear();
        std::string line;
        std::getline(file, line);  // skip header

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            std::vector<float> props;

            while (std::getline(ss, token, ',')) {
                if (!token.empty()) props.push_back(std::stof(token));
            }

            if (props.size() != 9) {
                std::cerr << "Invalid soil data line, skipping: " << line << "\n";
                continue;
            }

            Tile tile;
            tile.position.x = props[0];
            tile.position.y = props[1];
            tile.soilBaseQuality = props[2];  // already in CSV
            tile.sunlight = props[3];
            tile.nutrients = props[4];
            tile.pH = props[5];
            tile.organicMatter = props[6];
            tile.compaction = props[7];
            tile.salinity = props[8];

            std::size_t key = hashTwoFloats(tile.position.x, tile.position.y);
            tile_map_[key] = tile;
        }

        std::cout << "Loaded " << tile_map_.size() << " tiles.\n";
        return true;
    }

    // Load crop simulation data, keep only crop with lowest TTM for each tile
    bool updateCropData(const std::string& filename = "simulation_output.csv") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << filename << ": " << strerror(errno) << "\n";
            return false;
        }

        crop_map_.clear();
        std::string line;
        std::getline(file, line);  // skip header

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> fields;

            while (std::getline(ss, token, ',')) {
                if (!token.empty()) fields.push_back(token);
            }

            if (fields.size() < 6) continue;

            CropSimulation sim;
            sim.x = std::stof(fields[1]);
            sim.y = std::stof(fields[2]);
            sim.cropName = fields[3];
            sim.timeToMature = std::stod(fields[5]);

            std::size_t key = hashTwoFloats(sim.x, sim.y);
            auto it = crop_map_.find(key);
            if (it == crop_map_.end() || sim.timeToMature <= it->second.timeToMature) {
                crop_map_[key] = sim;
            }
        }

        std::cout << "Loaded " << crop_map_.size() << " crop simulations.\n";
        return true;
    }

    // Get the crop with lowest TTM for a specific tile
    std::string getCropWithLowestTTM(float x, float y) const {
        std::size_t key = hashTwoFloats(x, y);
        auto it = crop_map_.find(key);
        if (it != crop_map_.end()) return it->second.cropName;
        return "Unknown Crop";
    }

    // Get best crop for a rectangular area
    std::string getBestCropForArea(Point topLeft, Point bottomRight) const {
        std::unordered_map<std::string, int> cropCounts;

        for (const auto& pair : tile_map_) {
            const Tile& tile = pair.second;
            if (tile.position.x >= topLeft.x && tile.position.x <= bottomRight.x && tile.position.y >= topLeft.y &&
                tile.position.y <= bottomRight.y) {
                std::string crop = getCropWithLowestTTM(tile.position.x, tile.position.y);
                cropCounts[crop]++;
            }
        }

        // Find crop with max count
        std::string bestCrop = "Unknown Crop";
        int maxCount = 0;
        for (const auto& p : cropCounts) {
            if (p.second > maxCount) {
                maxCount = p.second;
                bestCrop = p.first;
            }
        }

        return bestCrop;
    }

   private:
    std::unordered_map<std::size_t, Tile> tile_map_;
    std::unordered_map<std::size_t, CropSimulation> crop_map_;
    float precision_;
};

}  // namespace Harvestor

#endif
