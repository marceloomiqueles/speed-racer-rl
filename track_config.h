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
    std::string repo_id;
    std::string repo_name;
    float lat;
    float lon;
    bool is_stub;
    std::string image_path;
    int screen_width;
    int screen_height;
    Vector2 spawn_position;
    float spawn_angle;
    std::vector<TrackCheckpoint> checkpoints;
};

inline std::vector<TrackCheckpoint> SandboxCheckpoints() {
    return {
        {{450.0f, 35.0f},  {450.0f, 150.0f}},
        {{719.0f, 260.0f}, {850.0f, 260.0f}},
        {{850.0f, 665.0f}, {723.0f, 665.0f}},
        {{523.0f, 482.0f}, {625.0f, 517.0f}},
        {{409.0f, 438.0f}, {295.0f, 413.0f}},
        {{160.0f, 730.0f}, {220.0f, 815.0f}},
        {{138.0f, 600.0f}, {49.0f, 600.0f}},
        {{138.0f, 205.0f}, {49.0f, 205.0f}}
    };
}

inline TrackConfig BuildSandboxTrack() {
    TrackConfig t;
    t.name = "sandbox";
    t.repo_id = "local-sandbox";
    t.repo_name = "Local Sandbox";
    t.lat = 0.0f;
    t.lon = 0.0f;
    t.is_stub = false;
    t.image_path = "assets/raceTrackFullyWalled.png";
    t.screen_width = 900;
    t.screen_height = 900;
    t.spawn_position = {430.0f, 92.0f};
    t.spawn_angle = 0.0f;
    t.checkpoints = SandboxCheckpoints();
    return t;
}

inline TrackConfig BuildF1StubTrack(
    const std::string& slug,
    const std::string& repoId,
    const std::string& repoName,
    float lat,
    float lon
) {
    TrackConfig t = BuildSandboxTrack();
    t.name = slug;
    t.repo_id = repoId;
    t.repo_name = repoName;
    t.lat = lat;
    t.lon = lon;
    t.is_stub = true;
    return t;
}

inline const std::vector<TrackConfig>& GetTrackCatalog() {
    static const std::vector<TrackConfig> kTracks = {
        BuildSandboxTrack(),

        BuildF1StubTrack("australian-gp", "au-1953", "Albert Park Circuit", -37.846f, 144.970f),
        BuildF1StubTrack("chinese-gp", "cn-2004", "Shanghai International Circuit", 31.340f, 121.221f),
        BuildF1StubTrack("japanese-gp", "jp-1962", "Suzuka International Racing Course", 34.844f, 136.534f),
        BuildF1StubTrack("bahrain-gp", "bh-2002", "Bahrain International Circuit", 26.031f, 50.512f),
        BuildF1StubTrack("saudi-arabian-gp", "sa-2021", "Jeddah Corniche Circuit", 21.632f, 39.104f),
        BuildF1StubTrack("miami-gp", "us-2022", "Miami International Autodrome", 25.958f, -80.239f),
        BuildF1StubTrack("canadian-gp", "ca-1978", "Circuit Gilles-Villeneuve", 45.506f, -73.525f),
        BuildF1StubTrack("monaco-gp", "mc-1929", "Circuit de Monaco", 43.737f, 7.429f),
        BuildF1StubTrack("spanish-gp", "es-1991", "Circuit de Barcelona-Catalunya", 41.569f, 2.259f),
        BuildF1StubTrack("austrian-gp", "at-1969", "Red Bull Ring", 47.223f, 14.761f),
        BuildF1StubTrack("british-gp", "gb-1948", "Silverstone Circuit", 52.072f, -1.017f),
        BuildF1StubTrack("belgian-gp", "be-1925", "Circuit de Spa-Francorchamps", 50.436f, 5.971f),
        BuildF1StubTrack("hungarian-gp", "hu-1986", "Hungaroring", 47.583f, 19.250f),
        BuildF1StubTrack("dutch-gp", "nl-1948", "Circuit Zandvoort", 52.389f, 4.541f),
        BuildF1StubTrack("italian-gp", "it-1922", "Autodromo Nazionale Monza", 45.621f, 9.290f),
        BuildF1StubTrack("spanish-gp-madrid", "es-2026", "Circuito de Madring", 40.471f, -3.620f),
        BuildF1StubTrack("azerbaijan-gp", "az-2016", "Baku City Circuit", 40.369f, 49.842f),
        BuildF1StubTrack("singapore-gp", "sg-2008", "Marina Bay Street Circuit", 1.291f, 103.859f),
        BuildF1StubTrack("united-states-gp", "us-2012", "Circuit of the Americas", 30.135f, -97.633f),
        BuildF1StubTrack("mexico-city-gp", "mx-1962", "Autodromo Hermanos Rodriguez", 19.402f, -99.091f),
        BuildF1StubTrack("sao-paulo-gp", "br-1940", "Autodromo Jose Carlos Pace - Interlagos", -23.702f, -46.698f),
        BuildF1StubTrack("las-vegas-gp", "us-2023", "Las Vegas Street Circuit", 36.116f, -115.168f),
        BuildF1StubTrack("qatar-gp", "qa-2004", "Losail International Circuit", 25.490f, 51.454f),
        BuildF1StubTrack("abu-dhabi-gp", "ae-2009", "Yas Marina Circuit", 24.471f, 54.601f)
    };
    return kTracks;
}

inline std::vector<std::string> GetAvailableTrackNames() {
    std::vector<std::string> names;
    const auto& tracks = GetTrackCatalog();
    names.reserve(tracks.size());
    for (const auto& track : tracks) {
        names.push_back(track.name);
    }
    return names;
}

inline bool LoadTrackConfig(const std::string& trackName, TrackConfig& outConfig, std::string& error) {
    for (const auto& track : GetTrackCatalog()) {
        if (track.name == trackName) {
            outConfig = track;
            return true;
        }
    }
    error = "Unknown track: " + trackName;
    return false;
}
