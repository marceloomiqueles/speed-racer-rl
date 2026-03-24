// racing_trainer.cpp.
#include "raylib.h"
#include "dqn.h"
#include "replay_buffer.h"
#include "track_config.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <csignal>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <cctype>
#include <ctime>

// Ctrl+C support.
volatile sig_atomic_t interrupted = 0;

void signal_handler(int signal) {
    (void)signal;
    interrupted = 1;
}

// Track helpers.
bool IsWall(Color color)  { return (color.r == 15 && color.g == 15 && color.b == 15); }
bool IsTrack(Color color) { return (color.r == 35 && color.g == 35 && color.b == 35); }
bool IsGrass(Color color) { return (color.r == 34 && color.g == 177 && color.b == 76); }

float GetFrictionMultiplier(Color color) {
    if (IsWall(color))  return 999.0f;
    if (IsGrass(color)) return 3.0f;
    if (IsTrack(color)) return 1.0f;
    return 1.0f;
}

struct Checkpoint {
    Vector2 start;
    Vector2 end;
    bool crossed;

    bool CheckCrossing(Vector2 prevPos, Vector2 currentPos) {
        float x1 = prevPos.x, y1 = prevPos.y;
        float x2 = currentPos.x, y2 = currentPos.y;
        float x3 = start.x,  y3 = start.y;
        float x4 = end.x,    y4 = end.y;

        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (fabs(denom) < 0.001f) return false;

        float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

        return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
    }
};

static const char* ActionLabel(int action) {
    switch (action) {
        case 0: return "forward";
        case 1: return "reverse";
        case 2: return "left";
        case 3: return "right";
        case 4: return "forward+left";
        case 5: return "forward+right";
        case 6: return "idle";
        default: return "unknown";
    }
}

static void DrawControlTriangles(int cx, int cy, float s, int action) {
    const Color outline = BLACK;
    const Color noFill = BLANK;
    const bool forwardActive = (action == 0 || action == 4 || action == 5);
    const bool reverseActive = (action == 1);
    const bool leftActive = (action == 2 || action == 4);
    const bool rightActive = (action == 3 || action == 5);
    const Color forwardFill = forwardActive ? GREEN : noFill;
    const Color reverseFill = reverseActive ? RED : noFill;
    const Color leftFill = leftActive ? YELLOW : noFill;
    const Color rightFill = rightActive ? YELLOW : noFill;

    Vector2 upA = {(float)cx, (float)cy - s * 1.6f};
    Vector2 upB = {(float)cx - s, (float)cy - s * 0.2f};
    Vector2 upC = {(float)cx + s, (float)cy - s * 0.2f};

    Vector2 dnA = {(float)cx, (float)cy + s * 1.6f};
    Vector2 dnB = {(float)cx - s, (float)cy + s * 0.2f};
    Vector2 dnC = {(float)cx + s, (float)cy + s * 0.2f};

    Vector2 ltA = {(float)cx - s * 1.6f, (float)cy};
    Vector2 ltB = {(float)cx - s * 0.2f, (float)cy - s};
    Vector2 ltC = {(float)cx - s * 0.2f, (float)cy + s};

    Vector2 rtA = {(float)cx + s * 1.6f, (float)cy};
    Vector2 rtB = {(float)cx + s * 0.2f, (float)cy - s};
    Vector2 rtC = {(float)cx + s * 0.2f, (float)cy + s};

    auto draw_outline = [&](Vector2 a, Vector2 b, Vector2 c) {
        DrawLineEx(a, b, 2.0f, outline);
        DrawLineEx(b, c, 2.0f, outline);
        DrawLineEx(c, a, 2.0f, outline);
    };

    if (forwardFill.a > 0) DrawTriangle(upA, upB, upC, forwardFill);
    if (reverseFill.a > 0) DrawTriangle(dnA, dnB, dnC, reverseFill);
    if (leftFill.a > 0) DrawTriangle(ltA, ltB, ltC, leftFill);
    if (rightFill.a > 0) DrawTriangle(rtA, rtB, rtC, rightFill);

    draw_outline(upA, upB, upC);
    draw_outline(dnA, dnB, dnC);
    draw_outline(ltA, ltB, ltC);
    draw_outline(rtA, rtB, rtC);
}

static void DrawControlPadTopRight(int screenWidth, int action) {
    const int margin = 16;
    const int boxW = 110;
    const int boxH = 110;
    const int x = screenWidth - boxW - margin;
    const int y = margin;

    DrawRectangle(x, y, boxW, boxH, WHITE);
    DrawRectangleLinesEx({(float)x, (float)y, (float)boxW, (float)boxH}, 1.5f, BLACK);

    const int cx = x + boxW / 2;
    const int cy = y + boxH / 2;
    DrawControlTriangles(cx, cy, 18.0f, action);
}

enum class TraceState : unsigned char {
    Normal = 0,
    Grass = 1,
    WallHit = 2
};

struct TraceSample {
    Vector2 pos;
    TraceState state;
};

static void DrawRecentTrace(const std::vector<TraceSample>& trace) {
    if (trace.size() < 2) return;

    for (size_t i = 1; i < trace.size(); i++) {
        Color c = ColorAlpha(SKYBLUE, 0.35f);
        if (trace[i].state == TraceState::Grass) {
            c = ColorAlpha(ORANGE, 0.45f);
        } else if (trace[i].state == TraceState::WallHit) {
            c = ColorAlpha(RED, 0.60f);
        }
        DrawLineEx(trace[i - 1].pos, trace[i].pos, 2.0f, c);
    }
}

static std::vector<Checkpoint> BuildCheckpoints(const TrackConfig& track) {
    std::vector<Checkpoint> checkpoints;
    checkpoints.reserve(track.checkpoints.size());
    for (const auto& cp : track.checkpoints) {
        checkpoints.push_back({cp.start, cp.end, false});
    }
    return checkpoints;
}

struct AiLapRecord {
    double time_seconds = 1e18;
    std::string time_text;
    std::string model;
    std::string updated_at_utc;
    std::string source;
    bool clean_lap = false;
};

static std::string FormatLapSeconds(double sec) {
    if (sec < 0.0) return "";
    int minutes = (int)(sec / 60.0);
    double rem = sec - (double)minutes * 60.0;
    std::ostringstream oss;
    oss << minutes << ":" << std::fixed << std::setprecision(3) << std::setw(6) << std::setfill('0') << rem;
    return oss.str();
}

static std::string NowUtcIso8601() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

static std::filesystem::path ResolveAiRecordsPath() {
    namespace fs = std::filesystem;
    if (fs::exists("README.md")) return fs::path("tracks_ai_records.csv");
    if (fs::exists(fs::path("..") / "README.md")) return fs::path("..") / "tracks_ai_records.csv";
    return fs::path("tracks_ai_records.csv");
}

static std::unordered_map<std::string, AiLapRecord> LoadAiLapRecords(const std::filesystem::path& path) {
    std::unordered_map<std::string, AiLapRecord> out;
    std::ifstream in(path);
    if (!in.is_open()) return out;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("track_slug,", 0) == 0) continue;
        std::stringstream ss(line);
        std::string slug, sec, timeText, model, updated, source, cleanLap;
        if (!std::getline(ss, slug, ',')) continue;
        if (!std::getline(ss, sec, ',')) continue;
        if (!std::getline(ss, timeText, ',')) continue;
        if (!std::getline(ss, model, ',')) continue;
        if (!std::getline(ss, updated, ',')) continue;
        std::getline(ss, source, ',');
        std::getline(ss, cleanLap);
        try {
            AiLapRecord rec;
            rec.time_seconds = std::stod(sec);
            rec.time_text = timeText;
            rec.model = model;
            rec.updated_at_utc = updated;
            rec.source = source;
            rec.clean_lap = (cleanLap == "true" || cleanLap == "1");
            out[slug] = rec;
        } catch (...) {
        }
    }
    return out;
}

static bool SaveAiLapRecords(const std::filesystem::path& path,
                             const std::unordered_map<std::string, AiLapRecord>& records) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "track_slug,time_seconds,time_text,model,updated_at_utc,source,clean_lap\n";
    std::vector<std::string> keys;
    keys.reserve(records.size());
    for (const auto& kv : records) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    for (const auto& slug : keys) {
        const auto& r = records.at(slug);
        out << slug << ","
            << std::fixed << std::setprecision(3) << r.time_seconds << ","
            << r.time_text << ","
            << r.model << ","
            << r.updated_at_utc << ","
            << r.source << ","
            << (r.clean_lap ? "true" : "false") << "\n";
    }
    return true;
}

static const std::vector<float>& LidarOffsetsShort() {
    static const std::vector<float> k = {
        -PI/2, -5*PI/12, -PI/3, -PI/4, -PI/6, -PI/12, 0.0f,
         PI/12, PI/6, PI/4, PI/3, 5*PI/12, PI/2
    };
    return k;
}

static const std::vector<float>& LidarOffsetsLong() {
    static const std::vector<float> k = {-PI/6, -PI/12, 0.0f, PI/12, PI/6};
    return k;
}

// LIDAR ray cast
float CastLIDARRay(const Image& trackImage, Vector2 position, float angle, float maxDistance) {
    float distance = 0.0f;
    const float step = 2.0f;

    while (distance < maxDistance) {
        float checkX = position.x + cos(angle) * distance;
        float checkY = position.y + sin(angle) * distance;

        int pixelX = (int)checkX;
        int pixelY = (int)checkY;

        if (pixelX < 0 || pixelX >= trackImage.width ||
            pixelY < 0 || pixelY >= trackImage.height) {
            return distance;
        }

        Color pixel = GetImageColor(trackImage, pixelX, pixelY);
        if (IsWall(pixel)) return distance;

        distance += step;
    }

    return maxDistance;
}

// State: 5 base + 13 short-range lidar(danger) + 5 long-range anticipation (distance) = 23 dims.
std::vector<float> GetState(const Image& trackImage, Vector2 position, float angle, float speed) {
    std::vector<float> state;
    state.reserve(5 + 13 + 5);

    const float MAX_SPEED = 300.0f;
    state.push_back(speed / MAX_SPEED);

    state.push_back(sin(angle));
    state.push_back(cos(angle));

    state.push_back(position.x / (float)trackImage.width);
    state.push_back(position.y / (float)trackImage.height);

// Short-range rays 
    const float LIDAR_RANGE = 200.0f;
    const float REFERENCE_DIST = 50.0f; // Distance at which danger ~= 1.0.

    for (float offset : LidarOffsetsShort()) {
        float d = CastLIDARRay(trackImage, position, angle + offset, LIDAR_RANGE);

// Inverse normalization: close walls = high value.
        float danger = 1.0f / ((d / REFERENCE_DIST) + 0.1f);
        float normalized = std::min(1.0f, danger);

        state.push_back(normalized);
    }

// Long-range anticipation rays 
// These help the network "see" a turn earlier without changing your short-range "danger" behavior.
    const float LONG_RANGE = 900.0f; // tune 700..1200 depending on track scale.

    for (float offset : LidarOffsetsLong()) {
        float d = CastLIDARRay(trackImage, position, angle + offset, LONG_RANGE);
        float norm = d / LONG_RANGE; // 0..1 where 1 means far/clear.
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        state.push_back(norm);
    }

    return state; // 23.
}

// Training statistics (kept in memory; CSVs are written per milestone window).
struct TrainingStats {
    std::vector<float> episode_rewards;
    std::vector<int>   episode_lengths;
    std::vector<float> episode_losses;
    std::vector<int>   episode_laps;
    std::vector<int>   episode_finishes; // 1 if finished all laps.
};

static inline float DistToCheckpointMid(const std::vector<Checkpoint>& checkpoints, int cpIndex, Vector2 p) {
    const Checkpoint& cp = checkpoints[cpIndex];
    Vector2 mid = { (cp.start.x + cp.end.x) * 0.5f, (cp.start.y + cp.end.y) * 0.5f };
    float dx = mid.x - p.x;
    float dy = mid.y - p.y;
    return sqrtf(dx*dx + dy*dy);
}

struct EvalResult {
    int episodes = 0;

    int finishes = 0; // number of full-race finishes.
    double finish_rate = 0.0; // finishes / episodes.

    double avg_laps = 0.0;
    double lap_gt1_rate = 0.0; // fraction of eval episodes with currentLap > 1.

    double avg_steps_finish = 0.0; // among finished episodes.
    double avg_steps_all = 0.0; // all episodes.

    double avg_wall_hits = 0.0; // all episodes.
    double avg_grass_frames = 0.0; // all episodes.

    double avg_score = 0.0; // all episodes.
};

// Greedy evaluation (epsilon=0) – used for best-model selection.
// IMPORTANT: action mapping MUST match training (case 1 is reverse, no braking hack).
static EvalResult EvaluateGreedy(
    DQN& dqn,
    const Image& trackImage,
    const std::vector<Checkpoint>& checkpointsTemplate,
    const Vector2& spawnPosition,
    float spawnAngle,
    bool startLapActive,
    int evalEpisodes,
    int max_steps,
    float DT
) {
// Physics constants (must match training).
    const float MAX_SPEED = 300.0f;
    const float ACCELERATION = 150.0f;
    const float FRICTION = 50.0f;
    const float TURN_SPEED_BASE = 3.0f;
    const float TURN_SPEED_FACTOR = 0.3f;
    const bool TERMINATE_ON_WALL_HIT = true;
    const double WALL_HIT_TERMINAL_SCORE_PENALTY = 5000.0;

// Scoring weights (tune later if you want).
    const float FINISH_BONUS = 100000.0f;
    const float STEP_PENALTY = 1.0f; // per step.
    const float WALL_HIT_PENALTY = 200.0f; // per hit.
    const float GRASS_PENALTY = 50.0f; // per frame on grass.

    EvalResult out;
    out.episodes = evalEpisodes;

    dqn.set_training_mode(false);

    long long sumLaps = 0;
    long long sumStepsAll = 0;

    int finishedCount = 0;
    int lapGt1Count = 0;
    long long sumStepsFinished = 0;

    long long sumWallHits = 0;
    long long sumGrassFrames = 0;

    double sumScore = 0.0;

    for (int ep = 0; ep < evalEpisodes; ep++) {
        std::vector<Checkpoint> checkpoints = checkpointsTemplate;
        for (auto& cp : checkpoints) cp.crossed = false;

        Vector2 position = spawnPosition;
        Vector2 velocity = {0, 0};
        float angle = spawnAngle;
        float speed = 0.0f;

        int currentLap = startLapActive ? 1 : -1;
        const int totalLaps = 3;
        int nextCheckpoint = startLapActive ? 1 : 0;
        bool raceFinished = false;
        int consecutiveWallHits = 0;

        int wallHits = 0;
        int grassFrames = 0;
        bool terminalWallHitEpisode = false;

        std::vector<float> state = GetState(trackImage, position, angle, speed);

        int steps = 0;
        while (!raceFinished && steps < max_steps) {
            Vector2 prevPosition = position;

            auto q_values = dqn.predict(state);
            int action = (int)(std::max_element(q_values.begin(), q_values.end()) - q_values.begin());

            float accelerationInput = 0.0f;
            float steeringInput = 0.0f;

            switch (action) {
                case 0: accelerationInput = 1.0f; break;
                case 1: accelerationInput = -0.4f; break;
                case 2: steeringInput = -1.0f; break;
                case 3: steeringInput =  1.0f; break;
                case 4: accelerationInput = 1.0f; steeringInput = -1.0f; break;
                case 5: accelerationInput = 1.0f; steeringInput =  1.0f; break;
                case 6: break;
            }
            // Allow "brake while turning" without expanding the action space.
            if (fabs(steeringInput) > 0.1f && accelerationInput == 0.0f && speed > 35.0f) {
                accelerationInput = -0.20f;
            }

            int checkPixelX = (int)position.x;
            int checkPixelY = (int)position.y;
            float surfaceFriction = 1.0f;

            if (checkPixelX >= 0 && checkPixelX < trackImage.width &&
                checkPixelY >= 0 && checkPixelY < trackImage.height) {
                Color surfaceColor = GetImageColor(trackImage, checkPixelX, checkPixelY);
                surfaceFriction = GetFrictionMultiplier(surfaceColor);
            }

            if (surfaceFriction > 2.0f) grassFrames++;

            speed += accelerationInput * ACCELERATION * DT;

            float frictionToApply = FRICTION;
            if (accelerationInput == 0.0f) frictionToApply = FRICTION * surfaceFriction;

            if (speed > 0) {
                speed -= frictionToApply * DT;
                if (speed < 0) speed = 0;
            } else if (speed < 0) {
                speed += frictionToApply * DT;
                if (speed > 0) speed = 0;
            }

            float maxSpeedOnSurface = MAX_SPEED;
            if (surfaceFriction > 2.0f) maxSpeedOnSurface = MAX_SPEED * 0.5f;

            if (speed > maxSpeedOnSurface) speed = maxSpeedOnSurface;
            if (speed < -maxSpeedOnSurface * 0.5f) speed = -maxSpeedOnSurface * 0.5f;

            float speedFactor = 1.0f / (1.0f + fabs(speed) / MAX_SPEED * TURN_SPEED_FACTOR);
            float turnRate = TURN_SPEED_BASE * speedFactor;

            if (fabs(speed) > 1.0f) angle += steeringInput * turnRate * DT * (speed / fabs(speed));

            velocity.x = cos(angle) * speed;
            velocity.y = sin(angle) * speed;

            position.x += velocity.x * DT;
            position.y += velocity.y * DT;

            int pixelX = (int)position.x;
            int pixelY = (int)position.y;

            bool hitWall = false;
            if (pixelX >= 0 && pixelX < trackImage.width &&
                pixelY >= 0 && pixelY < trackImage.height) {
                Color currentColor = GetImageColor(trackImage, pixelX, pixelY);
                if (IsWall(currentColor)) {
                    hitWall = true;
                    wallHits++;
                    position = prevPosition;
                    speed = 0.0f;
                }
            } else {
                hitWall = true;
                wallHits++;
                position = prevPosition;
                speed = 0.0f;
            }

            if (hitWall) {
                consecutiveWallHits++;
                if (TERMINATE_ON_WALL_HIT) {
                    terminalWallHitEpisode = true;
                }
            } else {
                consecutiveWallHits = 0;
            }

            if (!TERMINATE_ON_WALL_HIT && consecutiveWallHits >= 3) {
                Checkpoint& cpTarget = checkpoints[nextCheckpoint];
                Vector2 cpMid = {
                    (cpTarget.start.x + cpTarget.end.x) * 0.5f,
                    (cpTarget.start.y + cpTarget.end.y) * 0.5f
                };
                Vector2 toCp = {cpMid.x - position.x, cpMid.y - position.y};
                float toCpLen = sqrtf(toCp.x * toCp.x + toCp.y * toCp.y);
                if (toCpLen > 1e-3f) {
                    toCp.x /= toCpLen;
                    toCp.y /= toCpLen;
                    angle = atan2f(toCp.y, toCp.x);
                    position.x += toCp.x * 3.0f;
                    position.y += toCp.y * 3.0f;
                    speed = 20.0f;
                }
                consecutiveWallHits = 0;
            }

            Checkpoint& cp = checkpoints[nextCheckpoint];
            if (cp.CheckCrossing(prevPosition, position)) {
                if (nextCheckpoint == 0) {
                    if (currentLap > 0) {
                        bool allCrossed = true;
                        for (int i = 1; i < (int)checkpoints.size(); i++) {
                            if (!checkpoints[i].crossed) { allCrossed = false; break; }
                        }
                        if (allCrossed) {
                            cp.crossed = true;
                            currentLap++;

                            for (auto& c : checkpoints) c.crossed = false;
                            nextCheckpoint = 1;

                            if (currentLap >= totalLaps) raceFinished = true;
                        } else {
                            cp.crossed = false;
                        }
                    } else {
                        currentLap = 1;
                        cp.crossed = false;
                        nextCheckpoint = 1;
                    }
                } else {
                    if (currentLap > 0 && nextCheckpoint != 0) {
                        cp.crossed = true;
                        nextCheckpoint = (nextCheckpoint + 1) % (int)checkpoints.size();
                    } else {
                        cp.crossed = false;
                    }
                }
            }

            steps++;
            if (terminalWallHitEpisode) break;
            state = GetState(trackImage, position, angle, speed);
        }

        double score = 0.0;
        if (raceFinished) score += FINISH_BONUS;
        score -= (double)steps * STEP_PENALTY;
        score -= (double)wallHits * WALL_HIT_PENALTY;
        score -= (double)grassFrames * GRASS_PENALTY;
        if (terminalWallHitEpisode) {
            score -= WALL_HIT_TERMINAL_SCORE_PENALTY;
        }

        sumScore += score;
        sumLaps += currentLap;
        sumStepsAll += steps;

        sumWallHits += wallHits;
        sumGrassFrames += grassFrames;

        if (raceFinished) {
            finishedCount++;
            sumStepsFinished += steps;
        }
        if (currentLap > 1) {
            lapGt1Count++;
        }
    }

    out.finishes = finishedCount;
    out.finish_rate = (evalEpisodes > 0) ? ((double)finishedCount / (double)evalEpisodes) : 0.0;

    out.avg_laps = (evalEpisodes > 0) ? ((double)sumLaps / (double)evalEpisodes) : 0.0;
    out.lap_gt1_rate = (evalEpisodes > 0) ? ((double)lapGt1Count / (double)evalEpisodes) : 0.0;

    out.avg_steps_all = (evalEpisodes > 0) ? ((double)sumStepsAll / (double)evalEpisodes) : 0.0;
    out.avg_steps_finish = (finishedCount > 0) ? ((double)sumStepsFinished / (double)finishedCount) : 0.0;

    out.avg_wall_hits = (evalEpisodes > 0) ? ((double)sumWallHits / (double)evalEpisodes) : 0.0;
    out.avg_grass_frames = (evalEpisodes > 0) ? ((double)sumGrassFrames / (double)evalEpisodes) : 0.0;

    out.avg_score = (evalEpisodes > 0) ? (sumScore / (double)evalEpisodes) : 0.0;
    return out;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);

    enum class TrainingProfile {
        Auto,
        Base,
        Finetune
    };
    enum class CurriculumMode {
        Off,
        Auto
    };
    enum class CurriculumStage {
        Drive = 1,
        Clean = 2,
        Pace = 3,
        Corner = 4
    };

    int MILESTONE_FREQUENCY = 50;
    std::string trackName = "sandbox";
    std::string initModelPathArg;
    TrainingProfile profile = TrainingProfile::Auto;
    std::string profileLabel = "auto";
    CurriculumMode curriculumMode = CurriculumMode::Off;
    std::string curriculumLabel = "off";
    bool ENABLE_RENDER = false;
    bool ENABLE_RENDER_TRACE = false;
    bool ENABLE_RENDER_LIDAR = false;
    bool ENABLE_TOP_SPEED_REWARD = false;
    bool RESET_TRAINING = false;
    const int BATCH_SIZE = 32;
    const int REPLAY_BUFFER_SIZE = 50000;

    float LEARNING_RATE = 0.001f;

    const float GAMMA = 0.99f;
    float EPSILON_START = 1.0f;
    float EPSILON_END = 0.005f;
    float EPSILON_DECAY = 0.995f;
    int WARMUP_EPISODES = 5;
    float TOP_SPEED_REWARD_GAIN = 0.0030f;
    float TOP_SPEED_PEAK_BONUS = 0.3000f;

    const int TRAIN_EVERY_N_STEPS = 3;
    const int max_steps = 7500;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--track") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --track\n";
                return 1;
            }
            trackName = argv[++i];
        } else if (arg == "--profile") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --profile (expected: auto|base|finetune)\n";
                return 1;
            }
            std::string p = argv[++i];
            std::string lower = p;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            if (lower == "auto") {
                profile = TrainingProfile::Auto;
                profileLabel = "auto";
            } else if (lower == "base") {
                profile = TrainingProfile::Base;
                profileLabel = "base";
            } else if (lower == "finetune") {
                profile = TrainingProfile::Finetune;
                profileLabel = "finetune";
            } else {
                std::cerr << "Invalid value for --profile: " << p << " (expected: auto|base|finetune)\n";
                return 1;
            }
        } else if (arg == "--init-model") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --init-model\n";
                return 1;
            }
            initModelPathArg = argv[++i];
        } else if (arg == "--curriculum") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --curriculum (expected: off|auto)\n";
                return 1;
            }
            std::string c = argv[++i];
            std::string lower = c;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
            if (lower == "off") {
                curriculumMode = CurriculumMode::Off;
                curriculumLabel = "off";
            } else if (lower == "auto") {
                curriculumMode = CurriculumMode::Auto;
                curriculumLabel = "auto";
            } else {
                std::cerr << "Invalid value for --curriculum: " << c << " (expected: off|auto)\n";
                return 1;
            }
        } else if (arg == "--render") {
            ENABLE_RENDER = true;
        } else if (arg == "--render-trace") {
            ENABLE_RENDER_TRACE = true;
        } else if (arg == "--render-lidar") {
            ENABLE_RENDER_LIDAR = true;
        } else if (arg == "--reward-top-speed") {
            ENABLE_TOP_SPEED_REWARD = true;
        } else if (arg == "--reward-top-speed-gain") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --reward-top-speed-gain\n";
                return 1;
            }
            TOP_SPEED_REWARD_GAIN = std::stof(argv[++i]);
        } else if (arg == "--reward-top-speed-peak-bonus") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --reward-top-speed-peak-bonus\n";
                return 1;
            }
            TOP_SPEED_PEAK_BONUS = std::stof(argv[++i]);
        } else if (arg == "--reset-training") {
            RESET_TRAINING = true;
        } else if (arg == "--milestone") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --milestone\n";
                return 1;
            }
            MILESTONE_FREQUENCY = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: racing_trainer [--milestone <episodes>] [--track <track_name>] [--profile auto|base|finetune] [--init-model <path>] [--curriculum off|auto] [--render] [--render-trace] [--render-lidar] [--reward-top-speed] [--reward-top-speed-gain <float>] [--reward-top-speed-peak-bonus <float>] [--reset-training]\n";
            std::cout << "Example (base): racing_trainer --track sandbox --profile base --render\n";
            std::cout << "Example (finetune): racing_trainer --track australian-gp --profile finetune --init-model ../trainedModels/base/best.pt\n";
            std::cout << "Example (curriculum): racing_trainer --track sandbox --profile base --curriculum auto\n";
            std::cout << "Example (top-speed reward): racing_trainer --track sandbox --reward-top-speed --reward-top-speed-gain 0.003 --reward-top-speed-peak-bonus 0.30\n";
            return 0;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        } else {
            MILESTONE_FREQUENCY = std::atoi(arg.c_str());
        }
    }

    TrackConfig track;
    std::string trackError;
    if (!LoadTrackConfig(trackName, track, trackError)) {
        std::cerr << trackError << "\n";
        std::cerr << "Available tracks:\n";
        for (const auto& name : GetAvailableTrackNames()) {
            std::cerr << "  - " << name << "\n";
        }
        return 1;
    }

    if (profile == TrainingProfile::Auto) {
        if (track.name == "sandbox") {
            profile = TrainingProfile::Base;
            profileLabel = "base(auto)";
        } else {
            profile = TrainingProfile::Finetune;
            profileLabel = "finetune(auto)";
        }
    }

    if (profile == TrainingProfile::Base) {
        LEARNING_RATE = 0.001f;
        EPSILON_START = 1.0f;
        EPSILON_END = 0.005f;
        EPSILON_DECAY = 0.995f;
        WARMUP_EPISODES = 5;
    } else {
        LEARNING_RATE = 3e-4f;
        EPSILON_START = 0.25f;
        EPSILON_END = 0.005f;
        EPSILON_DECAY = 0.997f;
        WARMUP_EPISODES = 2;
    }

    struct StageParams {
        const char* name;
        float learning_rate;
        float epsilon_start;
        float epsilon_end;
        float epsilon_decay;
        float progress_gain;
        float speed_progress_gain;
        float wall_hit_penalty;
        float grass_penalty_rate;
        float step_penalty;
        float checkpoint_reward;
        float lap_reward;
        float clean_lap_bonus;
        float finish_reward;
        float corner_drive_bonus;
        float top_speed_reward_gain;
        float top_speed_peak_bonus;
    };
    auto get_stage_params = [&](CurriculumStage stage) -> StageParams {
        switch (stage) {
            case CurriculumStage::Drive:
                // Base stage: prioritize stability/no-collision before speed.
                return {"drive", 1.0e-3f, 1.00f, 0.030f, 0.998f, 0.08f, 0.0065f, 20.0f, 3.0f, 0.005f, 40.0f, 180.0f, 120.0f, 450.0f, 0.0f, 0.0f, 0.0f};
            case CurriculumStage::Clean:
                return {"clean", 5.0e-4f, 0.35f, 0.020f, 0.997f, 0.10f, 0.0085f, 14.0f, 2.5f, 0.006f, 62.0f, 300.0f, 220.0f, 550.0f, 0.0f, 0.0f, 0.0f};
            case CurriculumStage::Pace:
                return {"pace", 3.0e-4f, 0.20f, 0.010f, 0.998f, 0.12f, 0.0100f, 16.0f, 3.0f, 0.010f, 55.0f, 240.0f, 180.0f, 600.0f, 0.0f, 0.0030f, 0.30f};
            case CurriculumStage::Corner:
                return {"corner", 1.0e-4f, 0.10f, 0.005f, 0.999f, 0.13f, 0.0110f, 16.0f, 3.0f, 0.012f, 55.0f, 240.0f, 160.0f, 650.0f, 0.004f, 0.0040f, 0.35f};
        }
        return {"drive", LEARNING_RATE, EPSILON_START, EPSILON_END, EPSILON_DECAY, 0.10f, 0.0075f, 10.0f, 2.0f, 0.005f, 50.0f, 200.0f, 150.0f, 500.0f, 0.0f, 0.0f, 0.0f};
    };
    CurriculumStage curriculumStage = CurriculumStage::Drive;
    int curriculumStableEvals = 0;
    double curriculumStageEntryBestSteps = -1.0;
    if (curriculumMode == CurriculumMode::Auto) {
        StageParams sp = get_stage_params(curriculumStage);
        LEARNING_RATE = sp.learning_rate;
        EPSILON_START = sp.epsilon_start;
        EPSILON_END = sp.epsilon_end;
        EPSILON_DECAY = sp.epsilon_decay;
    }

    std::cout << "=== Racing DQN Training (CPU Optimized) ===\n";
    std::cout << "Milestone frequency: " << MILESTONE_FREQUENCY << " episodes\n";
    std::cout << "Track: " << track.name << "\n";
    std::cout << "Profile: " << profileLabel << "\n";
    std::cout << "Curriculum: " << curriculumLabel << "\n";
    if (!initModelPathArg.empty()) {
        std::cout << "Init model: " << initModelPathArg << "\n";
    }
    std::cout << "Car scale: " << std::fixed << std::setprecision(3) << track.car_scale << "\n";
    if (track.is_stub) {
        std::cout << "WARNING: track '" << track.name
                  << "' is a stub and currently uses sandbox geometry.\n";
    }
    std::cout << "Batch size: " << BATCH_SIZE << "\n";
    std::cout << "LR: " << std::scientific << LEARNING_RATE << std::fixed << "\n";
    std::cout << "Epsilon schedule: start=" << std::setprecision(3) << EPSILON_START
              << " end=" << EPSILON_END << " decay=" << EPSILON_DECAY << "\n";
    std::cout << "Warmup episodes: " << WARMUP_EPISODES << "\n";
    std::cout << "Render: " << (ENABLE_RENDER ? "on" : "off (headless)") << "\n";
    std::cout << "Render trace: " << ((ENABLE_RENDER && ENABLE_RENDER_TRACE) ? "on" : "off") << "\n";
    std::cout << "Render lidar: " << ((ENABLE_RENDER && ENABLE_RENDER_LIDAR) ? "on" : "off") << "\n";
    if (ENABLE_TOP_SPEED_REWARD) {
        std::cout << "Top-speed reward: on(manual) (gain="
                  << std::fixed << std::setprecision(4) << TOP_SPEED_REWARD_GAIN
                  << ", peak_bonus=" << TOP_SPEED_PEAK_BONUS << ")\n";
    } else if (curriculumMode == CurriculumMode::Auto) {
        std::cout << "Top-speed reward: auto-by-stage (off in drive/clean, on in pace/corner)\n";
    } else {
        std::cout << "Top-speed reward: off\n";
    }
    std::cout << "Reset training: " << (RESET_TRAINING ? "yes" : "no") << "\n";
    std::cout << "Press Ctrl+C to save and exit gracefully\n";
    std::cout << "==========================================\n\n";

    SetTraceLogLevel(LOG_ERROR);

    Image trackImage = LoadImage(track.image_path.c_str());
    if (trackImage.data == NULL) {
        std::cerr << "Failed to load track image!\n";
        return 1;
    }

    std::vector<Checkpoint> checkpointsTemplate = BuildCheckpoints(track);
    const size_t TRACE_MAX_POINTS = 100;
    const std::string profileStorageKey = (profile == TrainingProfile::Base) ? "base" : "finetune";

    const float MAX_SPEED = 300.0f;
    const float ACCELERATION = 150.0f;
    const float FRICTION = 50.0f;
    const float TURN_SPEED_BASE = 3.0f;
    const float TURN_SPEED_FACTOR = 0.3f;
    const float DT = 1.0f / 60.0f;

// UPDATED STATE SIZE: 5 + 13 + 5 = 23.
    const int STATE_SIZE = 23;
    const int ACTION_SIZE = 7;

    DQN dqn(STATE_SIZE, ACTION_SIZE, LEARNING_RATE, GAMMA);
    ReplayBuffer replay_buffer(REPLAY_BUFFER_SIZE);

    namespace fs = std::filesystem;
    fs::path modelsRoot = "models";
    fs::path trackModelDirLegacy = modelsRoot / track.name;
    fs::path trackProfileDir = trackModelDirLegacy / profileStorageKey;
    fs::path trackBestTimePath = trackProfileDir / "best_time.pt";
    fs::path trackStatePath = trackProfileDir / "training_state.txt";
    fs::path trackEvalHistoryPath = trackProfileDir / "eval_history.csv";
    fs::path externalModelsRoot = fs::path("..") / "trainedModels";
    fs::path externalTrackModelDirLegacy = externalModelsRoot / track.name;
    fs::path externalTrackProfileDir = externalTrackModelDirLegacy / profileStorageKey;
    fs::path externalBestTimePath = externalTrackProfileDir / "best_time.pt";
    fs::path externalLegacyBestTimePath = externalTrackModelDirLegacy / "best_time.pt";
    fs::path trackFinalPath = trackProfileDir / "model_final.pt";
    fs::path resumeModelPath;
    fs::path aiRecordsPath = ResolveAiRecordsPath();
    auto aiRecords = LoadAiLapRecords(aiRecordsPath);
    std::string aiRecordText = "AI Record: N/A";
    if (aiRecords.count(track.name) && aiRecords[track.name].time_seconds < 1e17) {
        aiRecordText = "AI Record: " + aiRecords[track.name].time_text + " (" + aiRecords[track.name].model + ")";
    }
    float lastEpisodeTopSpeed = 0.0f;

    std::error_code ec;
    if (RESET_TRAINING) {
        fs::remove_all(trackProfileDir, ec);
        if (ec) {
            std::cerr << "Failed to reset local track profile directory: " << trackProfileDir.string()
                      << " (" << ec.message() << ")\n";
            UnloadImage(trackImage);
            return 1;
        }

        ec.clear();
        fs::remove_all(externalTrackProfileDir, ec);
        if (ec) {
            std::cerr << "Failed to reset external track profile directory: " << externalTrackProfileDir.string()
                      << " (" << ec.message() << ")\n";
            UnloadImage(trackImage);
            return 1;
        }
        std::cout << "Training reset complete for track '" << track.name
                  << "' profile '" << profileStorageKey << "'.\n";
    }

    ec.clear();
    fs::create_directories(trackProfileDir, ec);
    if (ec) {
        std::cerr << "Failed to create model directory: " << trackProfileDir.string()
                  << " (" << ec.message() << ")\n";
        UnloadImage(trackImage);
        return 1;
    }

    ec.clear();
    fs::create_directories(trackModelDirLegacy, ec);
    if (ec) {
        std::cerr << "Warning: failed to create legacy model directory: "
                  << trackModelDirLegacy.string() << " (" << ec.message() << ")\n";
    }

    ec.clear();
    fs::create_directories(externalTrackProfileDir, ec);
    if (ec) {
        std::cerr << "Warning: failed to create external model directory: "
                  << externalTrackProfileDir.string() << " (" << ec.message() << ")\n";
    }

    ec.clear();
    fs::create_directories(externalTrackModelDirLegacy, ec);
    if (ec) {
        std::cerr << "Warning: failed to create external legacy model directory: "
                  << externalTrackModelDirLegacy.string() << " (" << ec.message() << ")\n";
    }

    auto find_latest_episode_model = [&](const fs::path& dir) -> fs::path {
        if (!fs::is_directory(dir)) return {};
        int bestEpisode = -1;
        fs::path bestPath;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            fs::path p = entry.path();
            if (p.extension() != ".pt") continue;
            std::string name = p.filename().string();
            const std::string prefix = "model_episode_";
            const std::string suffix = ".pt";
            if (name.rfind(prefix, 0) != 0) continue;
            if (name.size() <= prefix.size() + suffix.size()) continue;
            if (name.substr(name.size() - suffix.size()) != suffix) continue;
            std::string n = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
            try {
                int ep = std::stoi(n);
                if (ep > bestEpisode) {
                    bestEpisode = ep;
                    bestPath = p;
                }
            } catch (...) {
            }
        }
        return bestPath;
    };

    bool useFreshScheduler = false;
    if (RESET_TRAINING) {
        // Explicit reset always starts without implicit checkpoint fallback.
    } else if (!initModelPathArg.empty()) {
        resumeModelPath = initModelPathArg;
        useFreshScheduler = true;
    } else {
        fs::path latest = find_latest_episode_model(trackProfileDir);
        if (!latest.empty()) {
            resumeModelPath = latest;
        } else if (fs::is_regular_file(trackFinalPath)) {
            resumeModelPath = trackFinalPath;
        } else if (fs::is_regular_file(trackBestTimePath)) {
            resumeModelPath = trackBestTimePath;
        } else if (fs::is_regular_file(externalBestTimePath)) {
            resumeModelPath = externalBestTimePath;
        } else if (profile == TrainingProfile::Finetune && fs::is_regular_file(externalLegacyBestTimePath)) {
            // Backward compatibility with old path and replay default model location.
            resumeModelPath = externalLegacyBestTimePath;
        }
    }

    bool resumedFromCheckpoint = false;
    try {
        if (!resumeModelPath.empty()) {
            dqn.load_model(resumeModelPath.string());
            if (profile == TrainingProfile::Finetune) {
                dqn.set_learning_rate(1e-4f);
            } else {
                dqn.set_learning_rate(LEARNING_RATE);
            }
            resumedFromCheckpoint = true;
            std::cout << "Resumed from: " << resumeModelPath.string() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load checkpoint: " << e.what() << "\n";
        UnloadImage(trackImage);
        return 1;
    }

    if (!resumedFromCheckpoint) {
        std::cout << "No checkpoint found for track. Starting training from scratch.\n";
    }

    Texture2D trackTexture = {0};
    Texture2D carTexture = {0};
    bool showLidar = ENABLE_RENDER_LIDAR;
    if (ENABLE_RENDER) {
        InitWindow(track.screen_width, track.screen_height, "Speed Racer - Trainer Render");
        SetTargetFPS(60);
        trackTexture = LoadTextureFromImage(trackImage);
        carTexture = LoadTexture("assets/racecarTransparent.png");
    }
		
float epsilon = EPSILON_START;
    TrainingStats stats;
		
    auto training_start = std::chrono::steady_clock::now();

    int startEpisode = 1;
    double best_finish_rate = -1.0;
    double best_time_avg_steps_finish = 1e18;
    double best_score = -1e18;

    const int EVAL_EPISODES = 20;
    const int FINISH_RATE_MIN_IMPROVEMENT = 2;
    const double SCORE_MIN_IMPROVEMENT = 500.0;

    bool lr_dropped_once = false;
    bool lr_dropped_twice = false;

    auto stage_to_string = [&](CurriculumStage s) -> const char* {
        return get_stage_params(s).name;
    };
    auto apply_curriculum_stage = [&](CurriculumStage s, bool resetEpsilon) {
        StageParams sp = get_stage_params(s);
        dqn.set_learning_rate(sp.learning_rate);
        EPSILON_END = sp.epsilon_end;
        EPSILON_DECAY = sp.epsilon_decay;
        if (resetEpsilon) {
            epsilon = sp.epsilon_start;
        } else {
            epsilon = std::min(epsilon, sp.epsilon_start);
            if (epsilon < EPSILON_END) epsilon = EPSILON_END;
        }
        std::cout << "Curriculum stage '" << sp.name << "' active"
                  << " | LR=" << std::scientific << dqn.get_learning_rate()
                  << " | eps_start=" << std::fixed << std::setprecision(3) << sp.epsilon_start
                  << " | eps_end=" << sp.epsilon_end
                  << " | eps_decay=" << sp.epsilon_decay
                  << "\n";
    };

    if (!useFreshScheduler && fs::is_regular_file(trackStatePath)) {
        std::ifstream stateFile(trackStatePath);
        std::unordered_map<std::string, std::string> state;
        std::string line;
        while (std::getline(stateFile, line)) {
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            state[line.substr(0, pos)] = line.substr(pos + 1);
        }
        try {
            if (state.count("episode_next")) startEpisode = std::max(1, std::stoi(state["episode_next"]));
            if (state.count("epsilon")) epsilon = std::stof(state["epsilon"]);
            if (state.count("best_finish_rate")) best_finish_rate = std::stod(state["best_finish_rate"]);
            if (state.count("best_time_avg_steps_finish")) best_time_avg_steps_finish = std::stod(state["best_time_avg_steps_finish"]);
            if (state.count("best_score")) best_score = std::stod(state["best_score"]);
            if (state.count("lr_dropped_once")) lr_dropped_once = (state["lr_dropped_once"] == "1");
            if (state.count("lr_dropped_twice")) lr_dropped_twice = (state["lr_dropped_twice"] == "1");
            if (state.count("learning_rate")) dqn.set_learning_rate(std::stof(state["learning_rate"]));
            if (curriculumMode == CurriculumMode::Auto && state.count("curriculum_stage")) {
                int st = std::stoi(state["curriculum_stage"]);
                if (st >= (int)CurriculumStage::Drive && st <= (int)CurriculumStage::Corner) {
                    curriculumStage = (CurriculumStage)st;
                }
            }
            if (curriculumMode == CurriculumMode::Auto && state.count("curriculum_stable_evals")) {
                curriculumStableEvals = std::max(0, std::stoi(state["curriculum_stable_evals"]));
            }
            if (curriculumMode == CurriculumMode::Auto && state.count("curriculum_stage_entry_best_steps")) {
                curriculumStageEntryBestSteps = std::stod(state["curriculum_stage_entry_best_steps"]);
            }
            std::cout << "Resumed training state from: " << trackStatePath.string()
                      << " (next episode: " << startEpisode << ", epsilon: " << epsilon << ")\n";
        } catch (...) {
            std::cout << "Warning: invalid training state file, continuing with default scheduler state.\n";
        }
    } else if (useFreshScheduler && resumedFromCheckpoint) {
        std::cout << "Loaded init model with fresh scheduler state (episode=1, profile defaults).\n";
    } else if (resumedFromCheckpoint) {
        std::cout << "Checkpoint loaded, but no training_state.txt found. Continuing with default scheduler state.\n";
    }

    if (curriculumMode == CurriculumMode::Auto) {
        apply_curriculum_stage(curriculumStage, useFreshScheduler || startEpisode <= 1);
    }

    auto save_training_state = [&](int nextEpisode) {
        std::ofstream stateFile(trackStatePath);
        if (!stateFile.is_open()) {
            std::cerr << "Warning: failed to write training state file: " << trackStatePath.string() << "\n";
            return;
        }
        stateFile << "episode_next=" << nextEpisode << "\n";
        stateFile << "epsilon=" << epsilon << "\n";
        stateFile << "best_finish_rate=" << best_finish_rate << "\n";
        stateFile << "best_time_avg_steps_finish=" << best_time_avg_steps_finish << "\n";
        stateFile << "best_score=" << best_score << "\n";
        stateFile << "lr_dropped_once=" << (lr_dropped_once ? 1 : 0) << "\n";
        stateFile << "lr_dropped_twice=" << (lr_dropped_twice ? 1 : 0) << "\n";
        stateFile << "learning_rate=" << dqn.get_learning_rate() << "\n";
        stateFile << "curriculum_stage=" << (int)curriculumStage << "\n";
        stateFile << "curriculum_stable_evals=" << curriculumStableEvals << "\n";
        stateFile << "curriculum_stage_entry_best_steps=" << curriculumStageEntryBestSteps << "\n";
    };

    for (int episode = startEpisode; !interrupted; episode++) {
        std::vector<Checkpoint> checkpoints = checkpointsTemplate;
        for (auto& cp : checkpoints) cp.crossed = false;

        Vector2 position = track.spawn_position;
        Vector2 velocity = {0, 0};
        float angle = track.spawn_angle;
        float speed = 0.0f;

        const bool START_LAP_ACTIVE = (track.name != "sandbox");
        int currentLap = START_LAP_ACTIVE ? 1 : -1;
        const int totalLaps = 3;
        int nextCheckpoint = START_LAP_ACTIVE ? 1 : 0;
        bool raceFinished = false;
        float currentLapTime = 0.0f;

        float episode_reward = 0.0f;
        int episode_steps = 0;
        float total_loss = 0.0f;
        int loss_count = 0;
        float episodeTopSpeed = 0.0f;

        int stuckCounter = 0;
        Vector2 lastCheckPosition = position;
        int consecutiveWallHits = 0;
        int wallHitsThisLap = 0;
        int wallHitsThisEpisode = 0;
        const int MAX_WALL_HITS_BEFORE_DNF =
            (curriculumMode == CurriculumMode::Auto && curriculumStage == CurriculumStage::Drive) ? 6 : 1;
        const float WALL_HIT_TERMINAL_MULTIPLIER = 4.0f;

        int idleCounter = 0;
        const float V_IDLE = 8.0f;
        const int IDLE_GRACE_FRAMES = 30;
        const float IDLE_PENALTY = 0.02f;

        const int STUCK_CHECK_INTERVAL = 75;
        const float STUCK_DIST_THRESHOLD = 30.0f;
        const int STUCK_STRIKES_MAX = 3;
        const float STUCK_BREAK_PENALTY = 50.0f;
        StageParams stageParams = (curriculumMode == CurriculumMode::Auto)
            ? get_stage_params(curriculumStage)
            : StageParams{"manual", dqn.get_learning_rate(), EPSILON_START, EPSILON_END, EPSILON_DECAY, 0.10f, 0.0075f, 20.0f, 2.0f, 0.010f, 50.0f, 200.0f, 150.0f, 500.0f, 0.0f, 0.0f, 0.0f};

        std::vector<float> state = GetState(trackImage, position, angle, speed);
        std::vector<TraceSample> trace;
        if (ENABLE_RENDER && ENABLE_RENDER_TRACE) {
            trace.reserve(TRACE_MAX_POINTS);
            trace.push_back({position, TraceState::Normal});
        }
        if ((int)state.size() != STATE_SIZE) {
            std::cerr << "STATE SIZE MISMATCH at init: got=" << state.size()
                      << " expected=" << STATE_SIZE << "\n";
            UnloadImage(trackImage);
            return 1;
        }

        while (!raceFinished && episode_steps < max_steps && !interrupted) {
            bool terminalWallHit = false;
            if (ENABLE_RENDER && WindowShouldClose()) {
                interrupted = 1;
                break;
            }
            if (ENABLE_RENDER && IsKeyPressed(KEY_L)) {
                showLidar = !showLidar;
            }

            Vector2 prevPosition = position;
            currentLapTime += DT;

            if (episode_steps % STUCK_CHECK_INTERVAL == 0 && episode_steps > 0) {
                float dx = position.x - lastCheckPosition.x;
                float dy = position.y - lastCheckPosition.y;
                float distMoved = sqrtf(dx*dx + dy*dy);

                if (distMoved < STUCK_DIST_THRESHOLD) {
                    stuckCounter++;
                    if (stuckCounter >= STUCK_STRIKES_MAX) {
                        episode_reward -= STUCK_BREAK_PENALTY;
                        break;
                    }
                } else {
                    stuckCounter = 0;
                }
                lastCheckPosition = position;
            }

            int action = 0;
            if ((float)rand() / (float)RAND_MAX < epsilon) {
                action = rand() % ACTION_SIZE;
            } else {
                auto q_values = dqn.predict(state);
                action = (int)(std::max_element(q_values.begin(), q_values.end()) - q_values.begin());
            }

            float accelerationInput = 0.0f;
            float steeringInput = 0.0f;

            switch (action) {
                case 0: accelerationInput = 1.0f; break;
                case 1: accelerationInput = -0.4f; break;
                case 2: steeringInput = -1.0f; break;
                case 3: steeringInput =  1.0f; break;
                case 4: accelerationInput = 1.0f; steeringInput = -1.0f; break;
                case 5: accelerationInput = 1.0f; steeringInput =  1.0f; break;
                case 6: break;
            }
            // Allow "brake while turning" without expanding the action space.
            if (fabs(steeringInput) > 0.1f && accelerationInput == 0.0f && speed > 35.0f) {
                accelerationInput = -0.20f;
            }

            int checkPixelX = (int)position.x;
            int checkPixelY = (int)position.y;
            float surfaceFriction = 1.0f;

            if (checkPixelX >= 0 && checkPixelX < trackImage.width &&
                checkPixelY >= 0 && checkPixelY < trackImage.height) {
                Color surfaceColor = GetImageColor(trackImage, checkPixelX, checkPixelY);
                surfaceFriction = GetFrictionMultiplier(surfaceColor);
            }

            speed += accelerationInput * ACCELERATION * DT;

            float frictionToApply = FRICTION;
            if (accelerationInput == 0.0f) frictionToApply = FRICTION * surfaceFriction;

            if (speed > 0) {
                speed -= frictionToApply * DT;
                if (speed < 0) speed = 0;
            } else if (speed < 0) {
                speed += frictionToApply * DT;
                if (speed > 0) speed = 0;
            }

            float maxSpeedOnSurface = MAX_SPEED;
            if (surfaceFriction > 2.0f) maxSpeedOnSurface = MAX_SPEED * 0.5f;

            if (speed > maxSpeedOnSurface) speed = maxSpeedOnSurface;
            if (speed < -maxSpeedOnSurface * 0.5f) speed = -maxSpeedOnSurface * 0.5f;

            float speedFactor = 1.0f / (1.0f + fabs(speed) / MAX_SPEED * TURN_SPEED_FACTOR);
            float turnRate = TURN_SPEED_BASE * speedFactor;

            if (fabs(speed) > 1.0f) angle += steeringInput * turnRate * DT * (speed / fabs(speed));

            velocity.x = cos(angle) * speed;
            velocity.y = sin(angle) * speed;

            position.x += velocity.x * DT;
            position.y += velocity.y * DT;

            int pixelX = (int)position.x;
            int pixelY = (int)position.y;
            bool hitWall = false;

            if (pixelX >= 0 && pixelX < trackImage.width &&
                pixelY >= 0 && pixelY < trackImage.height) {
                Color currentColor = GetImageColor(trackImage, pixelX, pixelY);
                if (IsWall(currentColor)) {
                    hitWall = true;
                    position = prevPosition;
                    speed = 0.0f;
                }
            } else {
                hitWall = true;
                position = prevPosition;
                speed = 0.0f;
            }

            if (hitWall) {
                consecutiveWallHits++;
                wallHitsThisLap++;
                wallHitsThisEpisode++;
                if (wallHitsThisEpisode >= MAX_WALL_HITS_BEFORE_DNF) {
                    terminalWallHit = true;
                }
            } else {
                consecutiveWallHits = 0;
            }

            if (!terminalWallHit && consecutiveWallHits >= 3) {
                Checkpoint& cpTarget = checkpoints[nextCheckpoint];
                Vector2 cpMid = {
                    (cpTarget.start.x + cpTarget.end.x) * 0.5f,
                    (cpTarget.start.y + cpTarget.end.y) * 0.5f
                };
                Vector2 toCp = {cpMid.x - position.x, cpMid.y - position.y};
                float toCpLen = sqrtf(toCp.x * toCp.x + toCp.y * toCp.y);
                if (toCpLen > 1e-3f) {
                    toCp.x /= toCpLen;
                    toCp.y /= toCpLen;
                    angle = atan2f(toCp.y, toCp.x);
                    position.x += toCp.x * 3.0f;
                    position.y += toCp.y * 3.0f;
                    speed = 20.0f;
                }
                consecutiveWallHits = 0;
            }

            if (ENABLE_RENDER && ENABLE_RENDER_TRACE) {
                TraceState traceState = TraceState::Normal;
                if (hitWall) {
                    traceState = TraceState::WallHit;
                } else if (surfaceFriction > 2.0f) {
                    traceState = TraceState::Grass;
                }
                trace.push_back({position, traceState});
                if (trace.size() > TRACE_MAX_POINTS) {
                    trace.erase(trace.begin());
                }
            }

            float reward = 0.0f;

            float distToNextCP = DistToCheckpointMid(checkpoints, nextCheckpoint, position);
            float prevDistToNextCP = DistToCheckpointMid(checkpoints, nextCheckpoint, prevPosition);
            float progress = prevDistToNextCP - distToNextCP;
            reward += progress * stageParams.progress_gain;

            if (progress > 0.0f){
                reward += fabs(speed) * DT * stageParams.speed_progress_gain;
            }
            bool useTopSpeedReward =
                ENABLE_TOP_SPEED_REWARD ||
                (curriculumMode == CurriculumMode::Auto &&
                 (stageParams.top_speed_reward_gain > 0.0f || stageParams.top_speed_peak_bonus > 0.0f));
            float activeTopSpeedGain = ENABLE_TOP_SPEED_REWARD ? TOP_SPEED_REWARD_GAIN : stageParams.top_speed_reward_gain;
            float activeTopSpeedPeakBonus = ENABLE_TOP_SPEED_REWARD ? TOP_SPEED_PEAK_BONUS : stageParams.top_speed_peak_bonus;
            if (useTopSpeedReward && progress > 0.0f && !hitWall) {
                float absSpeed = fabs(speed);
                reward += absSpeed * DT * activeTopSpeedGain;
                if (absSpeed > episodeTopSpeed + 1.0f) {
                    reward += activeTopSpeedPeakBonus;
                }
            }
            if (fabs(speed) > episodeTopSpeed) {
                episodeTopSpeed = fabs(speed);
            }

            if (hitWall) {
                reward -= stageParams.wall_hit_penalty;
                if (terminalWallHit) {
                    reward -= stageParams.wall_hit_penalty * WALL_HIT_TERMINAL_MULTIPLIER;
                }
            }
            if (surfaceFriction > 2.0f) reward -= stageParams.grass_penalty_rate * DT;

            reward -= stageParams.step_penalty;
            if (stageParams.corner_drive_bonus > 0.0f &&
                fabs(steeringInput) > 0.1f &&
                accelerationInput > 0.5f &&
                speed > 40.0f &&
                !hitWall) {
                reward += fabs(speed) * DT * stageParams.corner_drive_bonus;
            }

            if (fabs(speed) < V_IDLE && progress <= 0.0f) {
                idleCounter++;
                if (idleCounter > IDLE_GRACE_FRAMES) reward -= IDLE_PENALTY;
            } else {
                idleCounter = 0;
            }

            Checkpoint& cp = checkpoints[nextCheckpoint];
            if (cp.CheckCrossing(prevPosition, position)) {
                if (nextCheckpoint == 0) {
                    if (currentLap > 0) {
                        bool allCrossed = true;
                        for (int i = 1; i < (int)checkpoints.size(); i++) {
                            if (!checkpoints[i].crossed) { allCrossed = false; break; }
                        }

                        if (allCrossed) {
                            float finishedLapTime = currentLapTime;
                            bool lapWasClean = (wallHitsThisLap == 0);
                            int lapIndexJustFinished = currentLap;
                            cp.crossed = true;
                            reward += stageParams.checkpoint_reward;
                            currentLap++;
                            reward += stageParams.lap_reward;
                            if (lapWasClean) {
                                reward += stageParams.clean_lap_bonus;
                            }
                            if (lapIndexJustFinished == 1) {
                                reward += 120.0f; // Helps exit the common "stuck at lap 1" plateau.
                            }
                            currentLapTime = 0.0f;
                            wallHitsThisLap = 0;

                            auto bestIt = aiRecords.find(track.name);
                            double prevBest = (bestIt != aiRecords.end()) ? bestIt->second.time_seconds : 1e18;
                            if (finishedLapTime > 0.0f && finishedLapTime < prevBest) {
                                AiLapRecord rec;
                                rec.time_seconds = (double)finishedLapTime;
                                rec.time_text = FormatLapSeconds(rec.time_seconds);
                                rec.model = profileStorageKey + "_training";
                                rec.updated_at_utc = NowUtcIso8601();
                                rec.source = "ai_training";
                                rec.clean_lap = lapWasClean;
                                aiRecords[track.name] = rec;
                                aiRecordText = "AI Record: " + rec.time_text + " (" + rec.model + ")";
                                if (SaveAiLapRecords(aiRecordsPath, aiRecords)) {
                                    std::cout << "★ AI lap record updated for " << track.name
                                              << ": " << rec.time_text << " (" << rec.time_seconds
                                              << "s) @ " << aiRecordsPath.string() << "\n";
                                } else {
                                    std::cout << "Warning: failed to write AI lap records to "
                                              << aiRecordsPath.string() << "\n";
                                }
                            }

                            for (auto& c : checkpoints) c.crossed = false;
                            nextCheckpoint = 1;

                            if (currentLap >= totalLaps) {
                                raceFinished = true;
                                reward += stageParams.finish_reward;
                            }
                        } else {
                            cp.crossed = false;
                        }
                    } else {
                        currentLap = 1;
                        cp.crossed = false;
                        nextCheckpoint = 1;
                    }
                } else {
                    if (currentLap > 0 && nextCheckpoint != 0) {
                        cp.crossed = true;
                        reward += stageParams.checkpoint_reward;
                        nextCheckpoint = (nextCheckpoint + 1) % (int)checkpoints.size();
                    } else {
                        cp.crossed = false;
                    }
                }
            }

            if (nextCheckpoint != 0) {
                Checkpoint& finishLine = checkpoints[0];
                if (finishLine.CheckCrossing(prevPosition, position)) reward -= 10.0f;
            }

            episode_reward += reward;
            episode_steps++;

            std::vector<float> next_state = GetState(trackImage, position, angle, speed);
            bool done = raceFinished || episode_steps >= max_steps || terminalWallHit;

            replay_buffer.add(state, action, reward, next_state, done);

            if (episode >= WARMUP_EPISODES &&
                replay_buffer.can_sample(BATCH_SIZE) &&
                (episode_steps % TRAIN_EVERY_N_STEPS == 0)) {

                std::vector<std::vector<float>> batch_states, batch_next_states;
                std::vector<int> batch_actions;
                std::vector<float> batch_rewards;
                std::vector<bool> batch_dones;

                replay_buffer.sample(BATCH_SIZE, batch_states, batch_actions,
                                    batch_rewards, batch_next_states, batch_dones);

                float loss = dqn.train(batch_states, batch_actions, batch_rewards,
                                    batch_next_states, batch_dones, BATCH_SIZE);
                total_loss += loss;
                loss_count++;
            }

            if (ENABLE_RENDER) {
                BeginDrawing();
                ClearBackground(RAYWHITE);

                if (trackTexture.id > 0) {
                    DrawTexture(trackTexture, 0, 0, WHITE);
                }

                if (ENABLE_RENDER_TRACE) {
                    DrawRecentTrace(trace);
                }

                for (int i = 0; i < (int)checkpoints.size(); i++) {
                    Color cpColor = (i == 0) ? RED : YELLOW;
                    if (checkpoints[i].crossed) cpColor = GREEN;
                    if (i == nextCheckpoint) cpColor = BLUE;
                    DrawLineEx(checkpoints[i].start, checkpoints[i].end, 3, cpColor);
                }

                if (showLidar) {
                    const float shortRange = 200.0f;
                    for (float off : LidarOffsetsShort()) {
                        float rayAngle = angle + off;
                        float d = CastLIDARRay(trackImage, position, rayAngle, shortRange);
                        Vector2 hitPoint = {
                            position.x + cosf(rayAngle) * d,
                            position.y + sinf(rayAngle) * d
                        };
                        DrawLineV(position, hitPoint, Fade(ORANGE, 0.35f));
                        DrawCircleV(hitPoint, 2.0f, ORANGE);
                    }

                    const float longRange = 900.0f;
                    for (float off : LidarOffsetsLong()) {
                        float rayAngle = angle + off;
                        float d = CastLIDARRay(trackImage, position, rayAngle, longRange);
                        Vector2 hitPoint = {
                            position.x + cosf(rayAngle) * d,
                            position.y + sinf(rayAngle) * d
                        };
                        DrawLineV(position, hitPoint, Fade(BLUE, 0.25f));
                        DrawCircleV(hitPoint, 2.0f, BLUE);
                    }
                }

                if (carTexture.id > 0) {
                    const float carTextureScale = track.car_scale;
                    Rectangle source = {0, 0, (float)carTexture.width, (float)carTexture.height};
                    Rectangle dest = {
                        position.x,
                        position.y,
                        carTexture.width * carTextureScale,
                        carTexture.height * carTextureScale
                    };
                    Vector2 origin = {
                        carTexture.width * carTextureScale / 2.0f,
                        carTexture.height * carTextureScale / 2.0f
                    };
                    DrawTexturePro(carTexture, source, dest, origin, angle * RAD2DEG, WHITE);
                } else {
                    DrawCircleV(position, 8.0f, RED);
                }

                DrawText(TextFormat("Episode: %d", episode), 10, 10, 20, RED);
                DrawText(TextFormat("Step: %d / %d", episode_steps, max_steps), 10, 32, 18, DARKGRAY);
                DrawText(TextFormat("Lap: %d / 3", currentLap), 10, 52, 18, DARKGRAY);
                DrawText(TextFormat("Speed: %.1f", fabs(speed)), 10, 72, 18, DARKGRAY);
                DrawText(TextFormat("Reward: %.2f", episode_reward), 10, 92, 18, DARKGRAY);
                DrawText(TextFormat("Epsilon: %.4f", epsilon), 10, 112, 18, DARKGRAY);
                DrawText(TextFormat("TopSpeed: %.1f", episodeTopSpeed), 10, 132, 18, DARKGRAY);
                DrawText(aiRecordText.c_str(), 10, 152, 18, DARKGREEN);
                DrawControlPadTopRight(track.screen_width, action);
                DrawText("Trainer Render Mode (L: lidar, ESC to stop training)", 10, track.screen_height - 28, 16, MAROON);
                EndDrawing();
            }

            state = std::move(next_state);
            if (done) break;
        }

        epsilon = std::max(EPSILON_END, epsilon * EPSILON_DECAY);

        stats.episode_rewards.push_back(episode_reward);
        stats.episode_lengths.push_back(episode_steps);
        stats.episode_losses.push_back(loss_count > 0 ? total_loss / loss_count : 0.0f);
        stats.episode_laps.push_back(currentLap);
        stats.episode_finishes.push_back(raceFinished ? 1 : 0);
        lastEpisodeTopSpeed = episodeTopSpeed;

        if (curriculumMode == CurriculumMode::Off && raceFinished && !lr_dropped_once) {
            dqn.set_learning_rate(3e-4f);
            lr_dropped_once = true;
            std::cout << "LR schedule: first finish detected. Lowering LR to " << dqn.get_learning_rate() << "\n";
        }

        if (curriculumMode == CurriculumMode::Off && !lr_dropped_twice && (int)stats.episode_finishes.size() >= 20) {
            int countFin = 0;
            for (int i = (int)stats.episode_finishes.size() - 20; i < (int)stats.episode_finishes.size(); i++) {
                countFin += stats.episode_finishes[i];
            }
            float finishRate20 = (float)countFin / 20.0f;
            if (finishRate20 >= 0.50f) {
                dqn.set_learning_rate(1e-4f);
                lr_dropped_twice = true;
                std::cout << "LR schedule: finishRate(last20)=" << finishRate20
                            << ". Lowering LR to " << dqn.get_learning_rate() << "\n";
            }
        }

        if (episode % 10 == 0) {
            float avg_reward = 0.0f;
            int window = std::min(10, (int)stats.episode_rewards.size());
            for (int i = 0; i < window; i++) {
                avg_reward += stats.episode_rewards[stats.episode_rewards.size() - 1 - i];
            }
            avg_reward /= window;

            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - training_start);

            std::cout << "Episode: " << episode
                        << " | Reward: " << std::fixed << std::setprecision(2) << episode_reward
                        << " | Avg(10): " << avg_reward
                        << " | Laps: " << currentLap
                        << " | ε: " << std::setprecision(3) << epsilon
                        << " | Steps: " << episode_steps
                        << " | TopSpeed: " << std::fixed << std::setprecision(1) << lastEpisodeTopSpeed
                        << " | LR: " << std::scientific << dqn.get_learning_rate()
                        << " | Stage: " << ((curriculumMode == CurriculumMode::Auto) ? stage_to_string(curriculumStage) : "manual")
                        << " | Time: " << std::fixed << duration.count() << "s"
                        << std::endl;
        }

        if (episode % MILESTONE_FREQUENCY == 0) {
            std::string model_path = (trackProfileDir / ("model_episode_" + std::to_string(episode) + ".pt")).string();
            dqn.save_model(model_path);

            std::string stats_path = (trackProfileDir / ("training_stats_" + std::to_string(episode) + ".csv")).string();
            std::ofstream stats_file(stats_path);

            stats_file << "episode,reward,length,avg_loss,laps,finished\n";

            int window = MILESTONE_FREQUENCY;
            int startEp = std::max(1, episode - window + 1);
            int endEp = episode;

            for (int ep = startEp; ep <= endEp; ep++) {
                // stats vectors are local to the current process run and start at startEpisode.
                int idx = ep - startEpisode;
                if (idx < 0 || idx >= (int)stats.episode_rewards.size()) continue;

                stats_file << ep << ","
                            << stats.episode_rewards[idx] << ","
                            << stats.episode_lengths[idx] << ","
                            << stats.episode_losses[idx] << ","
                            << stats.episode_laps[idx] << ","
                            << stats.episode_finishes[idx] << "\n";
            }
            stats_file.close();

            const int EVAL_MAX_STEPS = max_steps;

            auto eval = EvaluateGreedy(
                dqn,
                trackImage,
                checkpointsTemplate,
                track.spawn_position,
                track.spawn_angle,
                (track.name != "sandbox"),
                EVAL_EPISODES,
                EVAL_MAX_STEPS,
                DT
            );
            dqn.set_training_mode(true);

            std::cout << "\n✓ Milestone " << episode << " saved!\n";
            std::cout << "  Model: " << model_path << "\n";
            std::cout << "  Stats: " << stats_path << "\n";
            std::cout << "  Eval (greedy, " << EVAL_EPISODES << " eps)"
                        << " | finishes=" << eval.finishes << "/" << eval.episodes
                        << " (" << std::fixed << std::setprecision(1) << (eval.finish_rate * 100.0) << "%)"
                        << " | avg_laps=" << std::fixed << std::setprecision(2) << eval.avg_laps
                        << " | lap_gt1_rate=" << std::fixed << std::setprecision(1) << (eval.lap_gt1_rate * 100.0) << "%"
                        << " | avg_steps_all=" << std::fixed << std::setprecision(1) << eval.avg_steps_all
                        << " | avg_steps_finish=" << std::fixed << std::setprecision(1) << eval.avg_steps_finish
                        << " | avg_wall_hits=" << std::fixed << std::setprecision(2) << eval.avg_wall_hits
                        << " | avg_grass_frames=" << std::fixed << std::setprecision(1) << eval.avg_grass_frames
                        << " | avg_score=" << std::fixed << std::setprecision(1) << eval.avg_score
                        << "\n\n";

            {
                bool writeHeader = !fs::exists(trackEvalHistoryPath);
                std::ofstream evalHistory(trackEvalHistoryPath, std::ios::app);
                if (!evalHistory.is_open()) {
                    std::cerr << "Warning: failed to open eval history file: "
                              << trackEvalHistoryPath.string() << "\n";
                } else {
                    if (writeHeader) {
                        evalHistory << "timestamp_utc,episode,stage,finishes,eval_episodes,finish_rate,"
                                    << "avg_laps,lap_gt1_rate,avg_steps_all,avg_steps_finish,"
                                    << "avg_wall_hits,avg_grass_frames,avg_score,epsilon,learning_rate\n";
                    }
                    evalHistory << NowUtcIso8601() << ","
                                << episode << ","
                                << ((curriculumMode == CurriculumMode::Auto) ? stage_to_string(curriculumStage) : "manual") << ","
                                << eval.finishes << ","
                                << eval.episodes << ","
                                << std::fixed << std::setprecision(6) << eval.finish_rate << ","
                                << std::fixed << std::setprecision(6) << eval.avg_laps << ","
                                << std::fixed << std::setprecision(6) << eval.lap_gt1_rate << ","
                                << std::fixed << std::setprecision(6) << eval.avg_steps_all << ","
                                << std::fixed << std::setprecision(6) << eval.avg_steps_finish << ","
                                << std::fixed << std::setprecision(6) << eval.avg_wall_hits << ","
                                << std::fixed << std::setprecision(6) << eval.avg_grass_frames << ","
                                << std::fixed << std::setprecision(6) << eval.avg_score << ","
                                << std::fixed << std::setprecision(6) << epsilon << ","
                                << std::scientific << dqn.get_learning_rate() << "\n";
                }
            }

            bool save_finish_rate = false;
            int best_finishes_int = (int)std::round(best_finish_rate * (double)EVAL_EPISODES);
            if (best_finish_rate < 0.0) {
                save_finish_rate = true;
            } else if (eval.finishes >= best_finishes_int + FINISH_RATE_MIN_IMPROVEMENT) {
                save_finish_rate = true;
            } else if (eval.finish_rate > best_finish_rate && eval.finishes > best_finishes_int) {
                save_finish_rate = true;
            }

            if (save_finish_rate) {
                best_finish_rate = eval.finish_rate;
                std::string best_finish_path = (trackProfileDir / "best_finish_rate.pt").string();
                dqn.save_model(best_finish_path);
                std::cout << "★ Updated " << best_finish_path << " (finish_rate="
                        << std::fixed << std::setprecision(3) << best_finish_rate << ")\n";
            }

            bool save_time = false;
            if (eval.finishes > 0) {
                if (best_time_avg_steps_finish >= 1e17) {
                    save_time = true;
                } else if (eval.avg_steps_finish + 50.0 < best_time_avg_steps_finish) {
                    save_time = true;
                }
            }

            if (save_time) {
                best_time_avg_steps_finish = eval.avg_steps_finish;
                std::string best_time_path = trackBestTimePath.string();
                dqn.save_model(best_time_path);
                std::cout << "★ Updated " << best_time_path << " (avg_steps_finish="
                            << std::fixed << std::setprecision(1) << best_time_avg_steps_finish << ")\n";

                if (fs::is_directory(externalTrackProfileDir)) {
                    std::string external_best_time_path = externalBestTimePath.string();
                    dqn.save_model(external_best_time_path);
                    std::cout << "★ Exported " << external_best_time_path << "\n";
                }
                if (profile == TrainingProfile::Finetune && fs::is_directory(trackModelDirLegacy)) {
                    fs::path legacyBest = trackModelDirLegacy / "best_time.pt";
                    dqn.save_model(legacyBest.string());
                    std::cout << "★ Updated legacy replay path " << legacyBest.string() << "\n";
                }
                if (profile == TrainingProfile::Finetune && fs::is_directory(externalTrackModelDirLegacy)) {
                    dqn.save_model(externalLegacyBestTimePath.string());
                    std::cout << "★ Exported legacy replay path " << externalLegacyBestTimePath.string() << "\n";
                }
            }

            bool save_score = false;
            if (best_score <= -1e17) {
                save_score = true;
            } else if (eval.avg_score > best_score + SCORE_MIN_IMPROVEMENT) {
                save_score = true;
            } else if (eval.avg_score > best_score && eval.finish_rate > best_finish_rate) {
                save_score = true;
            }

            if (save_score) {
                best_score = eval.avg_score;
                std::string best_score_path = (trackProfileDir / "best_score.pt").string();
                dqn.save_model(best_score_path);
                std::cout << "★ Updated " << best_score_path << " (avg_score="
                        << std::fixed << std::setprecision(1) << best_score << ")\n";
            }

            if (curriculumMode == CurriculumMode::Auto) {
                bool promote = false;
                CurriculumStage nextStage = curriculumStage;

                if (curriculumStage == CurriculumStage::Drive) {
                    // Stage 1 goal: drive safely first (low collisions), then move to clean stage.
                    const double DRIVE_MAX_AVG_WALL_HITS = 0.80;
                    const double DRIVE_MIN_LAP_GT1_RATE = 0.20;
                    const double DRIVE_MAX_AVG_STEPS_ALL = 2600.0;
                    const int DRIVE_REQUIRED_STABLE_EVALS = 4;
                    if (eval.avg_wall_hits <= DRIVE_MAX_AVG_WALL_HITS &&
                        eval.lap_gt1_rate >= DRIVE_MIN_LAP_GT1_RATE &&
                        eval.avg_steps_all <= DRIVE_MAX_AVG_STEPS_ALL) {
                        curriculumStableEvals++;
                    } else {
                        curriculumStableEvals = 0;
                    }
                    if (curriculumStableEvals >= DRIVE_REQUIRED_STABLE_EVALS) {
                        promote = true;
                        nextStage = CurriculumStage::Clean;
                    }
                } else if (curriculumStage == CurriculumStage::Clean) {
                    // Stage 2 goal: clean finishes, low collision rate.
                    const double CLEAN_MIN_FINISH_RATE = 0.70;
                    const double CLEAN_MAX_AVG_WALL_HITS = 0.50;
                    const double CLEAN_MAX_AVG_STEPS_ALL = 2400.0;
                    if (eval.finish_rate >= CLEAN_MIN_FINISH_RATE &&
                        eval.avg_wall_hits <= CLEAN_MAX_AVG_WALL_HITS &&
                        eval.avg_steps_all <= CLEAN_MAX_AVG_STEPS_ALL) {
                        curriculumStableEvals++;
                    } else {
                        curriculumStableEvals = 0;
                    }
                    if (curriculumStableEvals >= 2) {
                        promote = true;
                        nextStage = CurriculumStage::Pace;
                    }
                } else if (curriculumStage == CurriculumStage::Pace) {
                    // Stage 3 goal: improve finish pace.
                    if (eval.finishes > 0 && eval.avg_steps_finish > 0.0) {
                        if (curriculumStageEntryBestSteps < 0.0) {
                            curriculumStageEntryBestSteps = eval.avg_steps_finish;
                        }
                        if (eval.finish_rate >= 0.70 &&
                            eval.avg_steps_finish <= curriculumStageEntryBestSteps * 0.95) {
                            curriculumStableEvals++;
                        } else {
                            curriculumStableEvals = 0;
                        }
                        if (eval.avg_steps_finish < curriculumStageEntryBestSteps) {
                            curriculumStageEntryBestSteps = eval.avg_steps_finish;
                        }
                    } else {
                        curriculumStableEvals = 0;
                    }
                    if (curriculumStableEvals >= 2) {
                        promote = true;
                        nextStage = CurriculumStage::Corner;
                    }
                }

                if (promote && nextStage != curriculumStage) {
                    curriculumStage = nextStage;
                    curriculumStableEvals = 0;
                    curriculumStageEntryBestSteps = -1.0;
                    apply_curriculum_stage(curriculumStage, true);
                    std::string stage_path = (trackProfileDir / ("stage_" + std::string(stage_to_string(curriculumStage)) + ".pt")).string();
                    dqn.save_model(stage_path);
                    std::cout << "▲ Curriculum promoted to stage '" << stage_to_string(curriculumStage)
                              << "' and checkpoint saved: " << stage_path << "\n";
                }
            }

            std::cout << "\n";
            save_training_state(episode + 1);
        }
    }

    if (interrupted) {
        std::cout << "\n\nInterrupted! Saving final model...\n";
        std::string final_path = (trackProfileDir / "model_final.pt").string();
        dqn.save_model(final_path);
        save_training_state(startEpisode + (int)stats.episode_rewards.size());
        std::cout << "Final model saved. Safe to exit.\n";
    }

    if (ENABLE_RENDER) {
        if (carTexture.id > 0) UnloadTexture(carTexture);
        if (trackTexture.id > 0) UnloadTexture(trackTexture);
        CloseWindow();
    }
	
    UnloadImage(trackImage);
    return 0;
}
