#pragma once

#include "raylib.h"

#include <string>
#include <vector>

struct TrackCheckpoint {
    Vector2 start;
    Vector2 end;
};

struct TrackConfig {
    std::string name;
    std::string image_path;
    int screen_width;
    int screen_height;
    Vector2 spawn_position;
    float spawn_angle;
    std::vector<TrackCheckpoint> checkpoints;
};

inline std::vector<std::string> GetAvailableTrackNames() {
    return {"sandbox"};
}

inline bool LoadTrackConfig(const std::string& trackName, TrackConfig& outConfig, std::string& error) {
    if (trackName == "sandbox") {
        outConfig.name = "sandbox";
        outConfig.image_path = "assets/raceTrackFullyWalled.png";
        outConfig.screen_width = 900;
        outConfig.screen_height = 900;
        outConfig.spawn_position = {430.0f, 92.0f};
        outConfig.spawn_angle = 0.0f;
        outConfig.checkpoints = {
            {{450.0f, 35.0f},  {450.0f, 150.0f}},
            {{719.0f, 260.0f}, {850.0f, 260.0f}},
            {{850.0f, 665.0f}, {723.0f, 665.0f}},
            {{523.0f, 482.0f}, {625.0f, 517.0f}},
            {{409.0f, 438.0f}, {295.0f, 413.0f}},
            {{160.0f, 730.0f}, {220.0f, 815.0f}},
            {{138.0f, 600.0f}, {49.0f, 600.0f}},
            {{138.0f, 205.0f}, {49.0f, 205.0f}}
        };
        return true;
    }

    error = "Unknown track: " + trackName;
    return false;
}

