#ifndef COMMON_HPP_
#define COMMON_HPP_

#include "config.hpp"

namespace Harvestor {

// ---------------- Crop ----------------
struct CropType {
    std::string name;
    float baseGrowthRate;  // normal growth speed
    sf::Color baseColor;
    float optimalWater;  // 0..1
    float tolerance;     // how much deviation it can handle
};

struct Crop {
    sf::RectangleShape shape;
    float growth = 0.f;
    sf::Vector2f originalSize;
};

// ---------------- Tile ----------------
struct Tile {
    sf::Vector2f position;
    float size;

    // Soil factors (0..1)
    float soilBaseQuality;  // fertility
    float waterLevel;       // current water
    float sunlight;         // sunlight exposure
    float nutrients;        // nutrient richness
    float pH;               // acidity (normalized 0..1)
    float organicMatter;    // organic content
    float compaction;       // soil compactness
    float salinity;         // optional, extra factor

    float soilQuality;  // final computed
    bool isInsideLand;

    Crop crop;
    bool hasCrop = false;

    float timeToMature = -1.f;  // -1 = not matured yet
};

struct Splash {
    sf::Vector2f position;
    float radius = 1.f;
    float maxRadius = 5.f;
    float alpha = 255.f;
    float speed = 40.f;

    bool update(float dt) {
        radius += speed * dt;
        alpha -= 150.f * dt;
        return (radius < maxRadius && alpha > 0.f);
    }

    void draw(sf::RenderWindow &window) {
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(position);
        circle.setFillColor(sf::Color(0, 150, 255, (sf::Uint8)alpha));
        window.draw(circle);
    }
};

struct Ripple {
    sf::Vector2f position;
    float radius = 2.f;
    float maxRadius = 5.f;
    float alpha = 200.f;
    float speed = 30.f;

    bool update(float dt) {
        radius += speed * dt;
        alpha -= 60.f * dt;
        return (radius < maxRadius && alpha > 0.f);
    }

    void draw(sf::RenderWindow &window) {
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(position);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineThickness(1.f);
        sf::Uint8 red = (sf::Uint8)(150 + (105 * (alpha / 255.f)));
        sf::Uint8 green = (sf::Uint8)(255 * (alpha / 255.f));
        sf::Uint8 blue = 255;
        circle.setOutlineColor(sf::Color(red, green, blue, (sf::Uint8)alpha));
        window.draw(circle);
    }
};

// ---------------- RainDrop ----------------
struct RainDrop {
    sf::Vector2f position;
    float speed;
};

// ---------------- BlobGenerator ----------------
class BlobGenerator {
   public:
    static sf::ConvexShape generate(float cx, float cy, float radius, int points = 25, float irregularity = 0.25f) {
        sf::ConvexShape shape;
        shape.setPointCount(points);
        for (int i = 0; i < points; i++) {
            float angle = i * 2 * M_PI / points;
            float r = radius * (1.f - irregularity / 2.f + static_cast<float>(rand()) / RAND_MAX * irregularity);
            shape.setPoint(i, sf::Vector2f(cx + r * cos(angle), cy + r * sin(angle)));
        }
        return shape;
    }
};

// ---------------- Utility ----------------
class Utility {
   public:
    static bool pointInPolygon(const sf::ConvexShape &polygon, const sf::Vector2f &point) {
        int n = polygon.getPointCount();
        for (int i = 0; i < n; i++) {
            sf::Vector2f a = polygon.getPoint(i) + polygon.getPosition();
            sf::Vector2f b = polygon.getPoint((i + 1) % n) + polygon.getPosition();
            float cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
            if (cross < 0) return false;
        }
        return true;
    }
};

// ---------------- Pond ----------------
class Pond {
   public:
    sf::ConvexShape shape;
    sf::Vector2f center;
    float radius;
    float quality;
    float quantity;

    Pond(float cx, float cy, float r, float q = 1.f, float qty = 1.f) : center(cx, cy), radius(r), quality(q), quantity(qty) {
        shape = BlobGenerator::generate(cx, cy, r, 25, 0.25f);
        shape.setFillColor(sf::Color(0, 150, 255));
    }

    void draw(sf::RenderWindow &window) { window.draw(shape); }
};

// Utility functions for RGB <-> HSL
struct ColorUtils {
    static void RGBtoHSL(const sf::Color &color, float &h, float &s, float &l) {
        float r = color.r / 255.f;
        float g = color.g / 255.f;
        float b = color.b / 255.f;

        float max = std::max({r, g, b});
        float min = std::min({r, g, b});
        l = (max + min) / 2.f;

        if (max == min) {
            h = s = 0.f;  // achromatic
        } else {
            float d = max - min;
            s = l > 0.5f ? d / (2.f - max - min) : d / (max + min);
            if (max == r)
                h = (g - b) / d + (g < b ? 6.f : 0.f);
            else if (max == g)
                h = (b - r) / d + 2.f;
            else
                h = (r - g) / d + 4.f;
            h /= 6.f;
        }
    }

    static sf::Color HSLtoRGB(float h, float s, float l) {
        auto hue2rgb = [](float p, float q, float t) -> float {
            if (t < 0.f) t += 1.f;
            if (t > 1.f) t -= 1.f;
            if (t < 1.f / 6.f) return p + (q - p) * 6.f * t;
            if (t < 1.f / 2.f) return q;
            if (t < 2.f / 3.f) return p + (q - p) * (2.f / 3.f - t) * 6.f;
            return p;
        };

        float r, g, b;
        if (s == 0.f)
            r = g = b = l;  // achromatic
        else {
            float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
            float p = 2.f * l - q;
            r = hue2rgb(p, q, h + 1.f / 3.f);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1.f / 3.f);
        }

        return sf::Color(static_cast<sf::Uint8>(r * 255.f), static_cast<sf::Uint8>(g * 255.f), static_cast<sf::Uint8>(b * 255.f));
    }
};

}  // namespace Harvestor

#endif