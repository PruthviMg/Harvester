#ifndef QUALITY_MATRIX_HPP_
#define QUALITY_MATRIX_HPP_

#include "common.hpp"

namespace Harvestor {
struct QualityMatrix {
    float cx, cy, radius;
    float tileSize;
    std::vector<Tile> tiles;

    QualityMatrix(float _cx, float _cy, float _r, float _tileSize = 6.f) : cx(_cx), cy(_cy), radius(_r), tileSize(_tileSize) {}

    std::vector<Tile> generateTiles() {
        tiles.clear();
        int samplesPerTile = 1;  // for simplicity
        float startX = cx - radius;
        float startY = cy - radius;
        for (float y = startY; y <= cy + radius; y += tileSize) {
            for (float x = startX; x <= cx + radius; x += tileSize) {
                float dx = x - cx;
                float dy = y - cy;
                if (std::sqrt(dx * dx + dy * dy) > radius) continue;  // inside circle only
                Tile t;
                t.position.x = x;
                t.position.y = y;
                t.soilBaseQuality = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.sunlight = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.nutrients = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.pH = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.organicMatter = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.compaction = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                t.salinity = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.7f;
                tiles.push_back(t);
            }
        }
        return tiles;
    }
};
}  // namespace Harvestor

#endif