#pragma once
#include <SFML/Graphics.hpp>
#include <limits>
#include <vector>

struct Normalizer {
    float minX, minY, maxX, maxY;
    float scale, offsetX, offsetY;
    float usableW, screenW, screenH;

    Normalizer(const std::vector<sf::Vector2f> &positions) {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        screenW = (float)desktop.width;
        screenH = (float)desktop.height;

        // UI offset: 15% left reserved
        usableW = screenW * 0.85f;
        float uiOffsetX = screenW * 0.15f;

        // Compute min/max
        minX = minY = std::numeric_limits<float>::max();
        maxX = maxY = std::numeric_limits<float>::lowest();
        for (auto &p : positions) {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);
        }

        float rangeX = maxX - minX;
        float rangeY = maxY - minY;

        if (rangeX == 0 || rangeY == 0) {
            scale = 1.f;
            offsetX = uiOffsetX;
            offsetY = 0;
            return;
        }

        float scaleX = usableW / rangeX;
        float scaleY = screenH / rangeY;
        scale = std::min(scaleX, scaleY);

        offsetX = uiOffsetX + (usableW - (rangeX * scale)) / 2.f;
        offsetY = (screenH - (rangeY * scale)) / 2.f;
    }

    sf::Vector2f normalize(const sf::Vector2f &p) const {
        float normX = (p.x - minX) * scale + offsetX;
        float normY = (p.y - minY) * scale + offsetY;
        return {normX, normY};
    }
};
