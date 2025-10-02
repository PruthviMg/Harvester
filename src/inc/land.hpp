#ifndef LAND_HPP_
#define LAND_HPP_

#include "common.hpp"

namespace Harvestor {
// ---------------- Land ----------------
class Land {
   public:
    sf::ConvexShape shape;
    sf::Vector2f center;
    float radius;
    std::vector<Tile> tiles;
    float tileSize;

    Land(float cx, float cy, float r, float tileSize = Config::landTileSize) : center(cx, cy), radius(r), tileSize(tileSize) {
        shape = BlobGenerator::generate(cx, cy, r, 25, 0.25f);
        shape.setFillColor(sf::Color(139, 69, 19));
    }

    void generateTiles(const std::vector<std::array<float, 7>> &soilMatrix = {}) {
        tiles.clear();
        int soilIndex = 0;

        float minX = center.x - radius;
        float maxX = center.x + radius;
        float minY = center.y - radius;
        float maxY = center.y + radius;

        for (float y = minY; y < maxY; y += tileSize) {
            for (float x = minX; x < maxX; x += tileSize) {
                sf::Vector2f tileCenter(x + tileSize / 2.f, y + tileSize / 2.f);
                if (!Utility::pointInPolygon(shape, tileCenter)) continue;

                Tile t;
                t.position = sf::Vector2f(x, y);
                t.size = tileSize;
                t.isInsideLand = true;
                t.hasCrop = false;

                if (!soilMatrix.empty()) {
                    const auto &vals = soilMatrix[soilIndex % soilMatrix.size()];  // repeat if needed
                    t.soilBaseQuality = vals[0];
                    t.sunlight = vals[1];
                    t.nutrients = vals[2];
                    t.pH = vals[3];
                    t.organicMatter = vals[4];
                    t.compaction = vals[5];
                    t.salinity = vals[6];
                    soilIndex++;
                } else {
                    std::cout << "soilMatrix is not available" << std::endl;
                }

                t.waterLevel = 0.f;
                tiles.push_back(t);
            }
        }
    }

    float computeSoilQuality(const Tile &tile, const CropType &crop) {
        float waterFactor = 1.f - std::abs(tile.waterLevel - crop.optimalWater) / crop.tolerance;
        waterFactor = std::clamp(waterFactor, 0.f, 1.f);

        // Weighted sum of factors
        float quality = 0.f;
        quality += 0.25f * tile.soilBaseQuality;
        quality += 0.15f * tile.sunlight;
        quality += 0.15f * tile.nutrients;
        quality += 0.1f * tile.pH;
        quality += 0.15f * tile.organicMatter;
        quality += 0.1f * (1.f - tile.compaction);  // lower compaction = better
        quality += 0.1f * (1.f - tile.salinity);    // lower salinity = better

        // Include water factor
        quality += 0.5f * waterFactor;

        return std::clamp(quality, 0.f, 1.f);
    }

    // ---------------- Update Tile Water ----------------
    void updateTileWater(Tile &tile, const std::vector<Pond> &ponds, float dt, const CropType &cropType) {
        // Gradually move water level toward optimal water from nearby ponds
        float targetWater = 0.f;

        for (auto &pond : ponds) {
            float dx = tile.position.x - pond.center.x;
            float dy = tile.position.y - pond.center.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Water contribution decreases with distance
            float waterFactor = std::max(0.f, 1.f - dist / (pond.radius * 5.f));
            targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
        }

        // Smoothly approach target water level
        float waterSpeed = 0.5f;  // change rate per second
        tile.waterLevel += (targetWater - tile.waterLevel) * waterSpeed * dt;

        // Evaporation
        float evaporation = 0.01f * dt;
        tile.waterLevel = std::max(tile.waterLevel - evaporation, 0.f);
    }

    void plantCrops(const CropType &cropType) {
        for (auto &tile : tiles) {
            float soilFactor = tile.soilBaseQuality;
            tile.hasCrop = true;
            tile.crop.growth = 0.f;

            tile.crop.originalSize = sf::Vector2f(tile.size, tile.size);

            sf::Vector2f initialSize = tile.crop.originalSize * Config::cropInitialScale;
            tile.crop.shape.setSize(initialSize);

            tile.crop.shape.setPosition(tile.position.x + (tile.size - initialSize.x) / 2, tile.position.y + (tile.size - initialSize.y) / 2);

            tile.crop.shape.setFillColor(cropType.baseColor);

            // INITIAL WATER: set close to crop optimal
            tile.waterLevel = cropType.optimalWater;

            // Compute soil quality initially
            tile.soilQuality = computeSoilQuality(tile, cropType);
        }
    }

    void updateGrowth(float dt, const std::vector<Pond> &ponds, const CropType &cropType, bool simulate, bool raining, float simTime) {
        if (!simulate) return;

        static std::mt19937 rng(12345);                          // fixed seed for reproducibility
        std::uniform_real_distribution<float> dist(0.9f, 1.1f);  // small variability

        for (auto &tile : tiles) {
            if (!tile.hasCrop) continue;

            // ---------------- Rain ----------------
            if (raining) {
                tile.waterLevel += Config::rainIntensity * dt;
                tile.waterLevel = std::clamp(tile.waterLevel, 0.f, 1.f);
            }

            // ---------------- Water from Ponds ----------------
            float targetWater = 0.f;
            for (const auto &pond : ponds) {
                float dx = tile.position.x - pond.center.x;
                float dy = tile.position.y - pond.center.y;
                float distToPond = std::sqrt(dx * dx + dy * dy);

                // Water contribution decreases with distance
                float waterFactor = std::max(0.f, 1.f - distToPond / (pond.radius * 5.f));
                targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
            }

            // Smoothly approach target water level
            float waterSpeed = 0.5f;  // rate per second
            tile.waterLevel += (targetWater - tile.waterLevel) * waterSpeed * dt;

            // Evaporation
            float evaporation = 0.01f * dt;
            tile.waterLevel = std::clamp(tile.waterLevel - evaporation, 0.f, 1.f);

            // ---------------- Soil Quality ----------------
            tile.soilQuality = computeSoilQuality(tile, cropType);

            // ---------------- Water Stress ----------------
            float waterDiff = tile.waterLevel - cropType.optimalWater;
            float waterStress = std::exp(-(waterDiff * waterDiff) / (2.f * cropType.tolerance * cropType.tolerance));
            waterStress = std::clamp(waterStress, 0.f, 1.f);

            // ---------------- Growth ----------------
            float growthRate = Config::growthSpeed * cropType.baseGrowthRate * tile.soilQuality * waterStress;
            growthRate *= dist(rng);  // add variability

            tile.crop.growth += growthRate * dt;
            tile.crop.growth = std::clamp(tile.crop.growth, 0.f, 1.f);

            // Record time of maturity
            if (tile.crop.growth >= 1.f && tile.timeToMature < 0.f) {
                tile.timeToMature = simTime;
            }

            // ---------------- Visual Scaling ----------------
            sf::Vector2f size =
                tile.crop.originalSize * (Config::cropInitialScale + (Config::cropMaxScale - Config::cropInitialScale) * tile.crop.growth);
            tile.crop.shape.setSize(size);
            tile.crop.shape.setPosition(tile.position.x + (tile.size - size.x) / 2, tile.position.y + (tile.size - size.y) / 2);

            // ---------------- Color Adjustment ----------------
            sf::Color targetColor = cropType.baseColor;

            // Darken fully grown crops slightly
            if (tile.crop.growth >= 1.f) {
                float darkFactor = 0.5f + 0.5f * (1.f - tile.crop.growth);
                float h, s, l;
                ColorUtils::RGBtoHSL(targetColor, h, s, l);
                l *= darkFactor;
                targetColor = ColorUtils::HSLtoRGB(h, s, l);
            }

            tile.crop.shape.setFillColor(targetColor);
        }
    }

    float getCropGrowthPercentage() const {
        int total = 0, grown = 0;
        for (auto &tile : tiles)
            if (tile.hasCrop) {
                total++;
                if (tile.crop.growth >= 1.f) grown++;
            }
        if (total == 0) return 0.f;
        return (float)grown / total * 100.f;
    }

    void draw(sf::RenderWindow &window, const CropType &cropType) {
        window.draw(shape);
        for (auto &tile : tiles)
            if (tile.hasCrop) window.draw(tile.crop.shape);
    }
};

}  // namespace Harvestor

#endif