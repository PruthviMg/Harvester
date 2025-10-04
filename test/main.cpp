#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Callback for libcurl to capture the response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    std::string api_key = "AIzaSyCQqddg4pZQYSav7av5G4jBE8ZRYUcxLMs";
    std::string readBuffer;

    try {
        // Step 1: Create JSON payload
        json payload = {
            {"model", "gemini-2.0-flash-001"},
            {"messages", {
                {{"role", "user"}, {"content", "Hello from C++!"}}
            }}
        };
        std::string payload_str = payload.dump();
        std::cout << "[DEBUG] JSON Payload: " << payload_str << std::endl;

        // Step 2: Initialize CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[ERROR] Failed to initialize CURL." << std::endl;
            return 1;
        }

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Optional: verbose debug
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        std::cout << "[DEBUG] Sending request..." << std::endl;
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) {
            std::cerr << "[ERROR] CURL request failed: " << curl_easy_strerror(res) << std::endl;
            return 1;
        }

        std::cout << "[DEBUG] Raw response: " << readBuffer << std::endl;

        // Step 3: Parse JSON response
        auto response = json::parse(readBuffer);
        if (!response.contains("choices") || response["choices"].empty()) {
            std::cerr << "[ERROR] Response does not contain 'choices'." << std::endl;
            return 1;
        }

        std::string reply = response["choices"][0]["message"]["content"];
        std::cout << "[INFO] LLM response: " << reply << std::endl;

    } catch (std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        std::cerr << "[DEBUG] Last received buffer: " << readBuffer << std::endl;
    }

    return 0;
}
