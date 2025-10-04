#ifndef EVALUATOR_HPP_
#define EVALUATOR_HPP_

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <filesystem>

#include "common.hpp"
namespace fs = std::filesystem;
using json = nlohmann::json;
namespace Harvestor {
constexpr auto KEY = "AIzaSyCQqddg4pZQYSav7av5G4jBE8ZRYUcxLMs";
class Evaluator {
   public:
    Evaluator(float precision = 0.001f) : precision_(precision) {}
    // Gemini LLM CSV analysis
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
    }
    void dumpLLMReport(const std::string& report_filename,
                   const std::string& analysis,
                   const std::string& bestCrop) const
    {
        std::ofstream file(report_filename);
        if (!file.is_open()) {
            std::cerr << "Failed to create report: " << report_filename << "\n";
            return;
        }

        file << "<!DOCTYPE html>\n<html>\n<head>\n";
        file << "<meta charset=\"UTF-8\">\n";
        file << "<title>Harvestor LLM Report</title>\n";
        file << "<style>\n";
        file << "body { font-family: Arial, sans-serif; padding: 20px; }\n";
        file << "h1 { color: #2E8B57; }\n";
        file << "pre { background: #f4f4f4; padding: 10px; border-radius: 5px; }\n";
        file << "</style>\n";
        file << "</head>\n<body>\n";

        file << "<h1>Harvestor LLM Analysis Report</h1>\n";
        file << "<h2>Soil Analysis</h2>\n<pre>" << analysis << "</pre>\n";
        file << "<h2>Best Crop for Area</h2>\n<p>" << bestCrop << "</p>\n";

        file << "</body>\n</html>\n";

        std::cout << "Report written to " << report_filename << std::endl;
    }

    std::string analyzeCSVWithLLM(const std::string& filename, const std::string& filename2, const std::string& api_key) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Cannot open CSV file: " << filename << "\n";
            return "Error opening CSV";
        }

        std::string csvContent;
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line) && lineCount < 100) {
            csvContent += line + "\n";
            lineCount++;
        }
        file.close();
        csvContent += "END OF 1st FILE\n\n";
        // Add note if the CSV is truncated
        if (!file.eof()) {
            csvContent += "\n[Note: Only the first 100 lines are sent to the LLM]\n";
        }
        std::ifstream file2(filename2);
        if (!file2.is_open()) {
            std::cerr << "[ERROR] Cannot open CSV file: " << filename2 << "\n";
            return "Error opening CSV";
        }

        line = {};
        lineCount = 0;
        while (std::getline(file2, line) && lineCount < 100) {
            csvContent += line + "\n";
            lineCount++;
        }
        file2.close();

        // Add note if the CSV is truncated
        if (!file.eof()) {
            csvContent += "\n[Note: Only the first 100 lines are sent to the LLM]\n";
        }
std::string prompt =
"You are a data analyst tasked with generating an HTML-only report based on two input datasets:\n\n"
"1. Dataset A (soil data): grid-based values such as soil_compatibility, nutrient levels, pH, or other soil metrics.\n"
"2. Dataset B (yield data): grid-based yield measurements, yield proxies, or quality metrics.\n\n"
"Instructions:\n\n"
"- Analyze the datasets together and generate insights only in HTML format. No plain text outside <html>.\n"
"- Structure the report with <h2> sections, <ul>/<li> bullet points, and <table> comparisons where needed.\n"
"- Highlight how soil factors from Dataset A relate to yield outcomes in Dataset B.\n\n"
"Sections to include:\n\n"
"1. Spatial Yield Variation\n"
"- Describe patterns of variation across coordinates (high vs low performing patches)\n"
"- Note ranges or clusters\n\n"
"2. Soil-Yield Relationship\n"
"- Explain correlations (e.g., higher soil_compatibility -> faster growth, better yield)\n"
"- Highlight thresholds where soil affects yield\n\n"
"3. Crop Comparison (if multiple crops)\n"
"- Compare robustness and sensitivity\n"
"- Identify which crop performs more consistently vs variable\n\n"
"4. Field Management Zones\n"
"- Identify zones for variable management (fertilizer, irrigation)\n"
"- Define rules for high-performing vs low-performing zones\n\n"
"5. Actionable Agronomy Insights\n"
"- Practical interventions based on thresholds (e.g., soil pH below 5.5 requires lime)\n"
"- Crop choice guidance\n\n"
"6. Business/Tech Impact\n"
"- Insights can power farm decision-support tools\n"
"- Suggest monetization (SaaS dashboards, consultancy)\n\n"
"7. Concrete Next Steps\n"
"- Generate heatmaps, run correlation analysis, define soil thresholds, build dashboard\n\n"
"Final output: Complete <html>...</html> report, structured and professional.";
        prompt += "\n\nHere are the datasets:\n\n" + csvContent ;
        json payload = {
            {"model", "gemini-2.0-flash-001"},
            {"messages", {
                {{"role", "user"}, {"content",prompt }}
            }}
        };


        std::string readBuffer;
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[ERROR] CURL init failed\n";
            return "CURL init error";
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL,
                         "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string payloadStr = payload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debug

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "[ERROR] CURL request failed: " << curl_easy_strerror(res) << "\n";
            return "CURL request error";
        }

        try {
            auto response = json::parse(readBuffer);
            if (response.contains("choices") && !response["choices"].empty()) {
                return response["choices"][0]["message"]["content"];
            }
            return "No analysis returned by LLM.";
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] JSON parse error: " << e.what() << "\n";
            return "JSON parse error";
        }
    }
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
            std::cout<<"Line: "<<line<<"\n";
            while (std::getline(ss, token, ',')) {
                std::cout<<"token: "<<token<<"\n";

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
                
            try{
                sim.x = std::stof(fields[1]);
                sim.y = std::stof(fields[2]);
                sim.cropName = fields[3];
                sim.timeToMature = std::stod(fields[5]);                         

                std::size_t key = hashTwoFloats(sim.x, sim.y);
                auto it = crop_map_.find(key);
                if (it == crop_map_.end() || sim.timeToMature <= it->second.timeToMature) {
                    crop_map_[key] = sim;
                }
            } catch (const std::exception &e) {
                std::cerr << "Invalid data in crop simulation: '" << line << "'\n";
                continue;
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
    std::string getBestCropForArea(Point topLeft, Point bottomRight)  {
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

        std::string analysis = analyzeCSVWithLLM("soil_data.csv", "simulation_output.csv", KEY);
        dumpLLMReport("harvestor_report.html",analysis, bestCrop);

        fs::path absolutePath = fs::absolute("/harvestor_report.html"); 
        auto command = std::string("start ") + absolutePath.string();
        system(command.c_str());

        return "Best crop for area: " + bestCrop + " Report : " + absolutePath.string();
    }

   private:
    std::unordered_map<std::size_t, Tile> tile_map_;
    std::unordered_map<std::size_t, CropSimulation> crop_map_;
    float precision_;
};

}  // namespace Harvestor

#endif
