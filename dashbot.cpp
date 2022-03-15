#include "HAPIH.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <string>
#include <cstring>
#include <algorithm>
#include <list>
#undef max

double MAX_ODDS = 4.0;
int FPS = 60;
int MAX_ATTEMPTS = 20;
#define ODDS ((bestLastJump == 0.0 ? (bestFitness / 2.0) : bestLastJump) / bestFitness) / (100.0 / MAX_ODDS) + 0.0001

/*
 * We can now read the mode, as well as some other stuff, with the offset GeometryDash.exe + 3222D0 + 164 + 224 + 638.
 * This is an "array" of booleans with the contents:
 * [0]: Is ship?
 * [1]: Is UFO? (interesting that it's earlier than ball, since UFO is newer)
 * [2]: Is ball?
 * [3]: Is wave?
 * [4]: Is robot?
 * [5]: Is spider?
 * [6]: Is upside-down?
 * [7]: Is dead?
 */

enum class Mode {
    cube,
    ship,
    ball,
    ufo,
    wave,
    robot,
    spider,
    unknown
};

const char * modeNames[] = {
    "cube",
    "ship",
    "ball",
    "ufo",
    "wave",
    "robot",
    "spider",
    "unknown"
};

bool modeFlying[] = {
    false,
    true,
    false,
    false,
    true,
    true,
    false,
    false
};

bool isDead = false;
Mode toMode(uint32_t hi, uint32_t lo) {
    isDead = hi & 0x1000000;
    hi &= 0xFFFF;
    if (hi == 0) {
        if (lo == 0) return Mode::cube;
        else if (lo == 0x1) return Mode::ship;
        else if (lo == 0x100) return Mode::ufo;
        else if (lo == 0x10000) return Mode::ball;
        else if (lo == 0x1000000) return Mode::wave;
        else return Mode::unknown;
    } else if (hi == 0x1) return Mode::robot;
    else if (hi == 0x100) return Mode::spider;
    else return Mode::unknown;
}

const std::unordered_map<std::string, double> oddsForAllMaps = {
    {"StereoMadness", 3.0},
    {"BackOnTrack", 4.0},
    {"Polargeist", 4.0},
    {"DryOut", 4.0},
    {"BaseAfterBase", 4.0},
    {"CantLetGo", 4.0},
    {"Jumper", 4.0},
    {"TimeMachine", 4.5},
    {"Cycles", 4.0},
    {"xStep", 5.0},
    {"Clutterfunk", 5.0},
    {"TheoryOfEverything", 5.0},
    {"ElectromanAdventures", 5.0},
    {"Clubstep", 6.0},
    {"Electrodynamix", 6.0},
    {"HexagonForce", 5.0},
    {"BlastProcessing", 4.0},
    {"TheoryOfEverything2", 6.0},
    {"GeometricalDominator", 5.0},
    {"Deadlocked", 6.0},
    {"Fingerdash", 5.0}
};

double bestFitness = 1;
double bestFitnessR = 0;
std::list<uint32_t> bestJumps;
double bestLastJump = 1;
int failCount = 0;
float lastX = 0;
std::ofstream out;
bool saving = false;
bool isDown = false;
Mode mode = Mode::cube;

void jump(const HackIH& GD, HWND window, bool state) {
    /*GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x611}, state);
    GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x612}, state);
    if (!state) GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x641}, state);
    if (GD.Read<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x228, 0x685}) || !state) {
        GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x228, 0x611}, state);
        GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x228, 0x612}, state);
        if (!state) GD.Write<bool>({GD.BaseAddress, 0x3222D0, 0x164, 0x228, 0x641}, state);
    }*/
    if (state) PostMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, 0);
    else PostMessage(window, WM_LBUTTONUP, 0, 0);
    Sleep(75);
}

void saveLevel(const char * argv[]) {
    if (!saving) return;
    bool loop = false;
    do {
        out.open(argv[2], std::ios_base::binary);
        out.write((char*)&bestFitness, sizeof(double));
        out.write((char*)&bestLastJump, sizeof(double));
        out.write(argv[1], strlen(argv[1]) + 1);
        for (uint32_t i : bestJumps) out.write((char*)&i, 4);
        out.put(0); out.put(0); out.put(0); out.put(0);
        long pos = out.tellp();
        out.close();
        // Rewrite the file if it gets corrupted
        if (pos < sizeof(double) * 2 + strlen(argv[1]) + 1 + bestJumps.size() * 4 + 4) loop = true;
    } while (loop);
}

int main(int argc, const char * argv[]) {
    if (argc > 1) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?") {
            std::cout << "DashBot is a simple AI designed to beat Geometry Dash levels. This version works with Geometry Dash 2.11.\nUsage: " << argv[0] << " [level name|jump speed] [save.dbj]\nType \"" << argv[0] << " list\" to list level names.\n";
            return 0;
        } else if (std::string(argv[1]) == "list") {
            std::cout << "List of known level names: ";
            for (std::pair<std::string, double> p : oddsForAllMaps) std::cout << p.first << ", ";
            std::cout << "\nTo use DashBot with another level, you can supply the jump speed (defaults to 4.0). A higher value will cause DashBot to jump more often. Use a higher value for faster/harder levels, and a lower value for slower levels.\n";
            return 0;
        } else if (oddsForAllMaps.find(argv[1]) != oddsForAllMaps.end()) {
            MAX_ODDS = oddsForAllMaps.at(argv[1]) * (60.0 / FPS);
        } else if (std::all_of(argv[1], argv[1] + strlen(argv[1]), [](char c)->bool{return isdigit(c) || c == '.';})) {
            MAX_ODDS = atof(argv[1]);
        } else {
            std::cerr << "Usage: " << argv[0] << " [level name|jump speed] [save.dbj] [fps]\nType \"" << argv[0] << " list\" to list level names\n";
            return 1;
        }
    }
    std::cout << "DashBot v3.2\nBased on Pizzabot-v4 by Pizzaroot\n";
    if (argc > 2) {
        saving = true;
        // file format:
        // double - bestFitness
        // double - bestLastJump
        // const char * - portal type
        // uint32_t* - bestJumps
        // uint32_t - 0
        std::ifstream in(argv[2], std::ios_base::binary);
        if (in.is_open()) {
            in.read((char*)&bestFitness, sizeof(double));
            in.read((char*)&bestLastJump, sizeof(double));
            while (in.get()) ; // ignore level type, we already know that
            uint32_t n = 0;
            while (!in.eof()) {
                in.read((char*)&n, 4);
                if (!n) break;
                bestJumps.push_back(n);
            }
            in.close();
        } else std::cerr << "Could not open input file, ignore this if you're creating a new save.\n";
    }
    if (argc > 3) {
        FPS = std::stoi(argv[3]);
    }
    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());
    HackIH GD;
    GD.bind("GeometryDash.exe");
    //std::list<uint32_t> jumps;
    std::list<uint32_t> newJumps;
    std::list<uint32_t>::iterator nextJump = bestJumps.begin();
    double lastJump = 0;
    bool randing = false;
    std::list<uint32_t> lastDeathPositions;
    float globalOffset = 0.0;
    float offsetDelta = 2.0;
    float lastY = 0.0;
    int attempt = 0;
    HWND window = FindWindow(NULL, "Geometry Dash");
    if (window == NULL) {
        std::cerr << "Could not find Geometry Dash window.\n";
        return 3;
    }
    /* === desync debugging === //
    float globalDesync = 0.0;
    RegisterHotKey(NULL, 1, MOD_CONTROL, VK_ADD);
    RegisterHotKey(NULL, 2, MOD_CONTROL, VK_SUBTRACT);
    // ======================== */
    RegisterHotKey(NULL, 3, MOD_CONTROL, VK_BACK);
    while (true) {
        float xPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x67C }) + globalOffset /*+ globalDesync*/; // === desync debugging ===
        if (xPos == lastX) continue;
        if (bestFitness > 100000) bestFitness = 1; // this may break very long levels
        //* === desync debugging === //
        MSG message;
        if (PeekMessage(&message, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {
            //if (message.wParam == 1) {globalDesync += 1.0; std::cout << "simulating desync by +1.0 (now " << globalDesync << ")\n";}
            //else if (message.wParam == 2) {globalDesync -= 1.0; std::cout << "simulating desync by -1.0 (now " << globalDesync << ")\n";}
            if (message.wParam == 3) {
                std::cout << "removing last jump from jump list\n";
                bestJumps.pop_back();
                if (bestJumps.empty()) bestLastJump = 1;
                else bestLastJump = bestJumps.back();
                bestFitnessR = bestFitness;
                bestFitness = bestLastJump;
                failCount = 0;
                //globalOffset = 0.0;
                offsetDelta = 2.0;
                saveLevel(argv);
                std::cout << "best fitness is now " << bestFitness << "\n";
            }
        }
        // ======================== */
        bool shouldJump = false;
        float yPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x680 });
        //if (xPos != lastX) {
            Mode oldmode = mode;
            mode = toMode(GD.Read<uint32_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x63C}), GD.Read<uint32_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x638}));
            if (mode != oldmode && xPos > lastX) {
                std::cout << "switched between modes (" << modeNames[(int)oldmode] << " -> " << modeNames[(int)mode] << ")\n";
                if (isDown) {
                    jump(GD, window, false);
                    isDown = false;
                }
            }
        //}
        bool canJump = GD.Read<uint8_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x640}) || GD.Read<uint8_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x66C, 0x20, 0});
        uint32_t percentage_int = GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x3C0, 0x12C });
        char percentage[4] = {percentage_int & 0xFF, (percentage_int >> 8) & 0xFF, (percentage_int >> 16) & 0xFF, (percentage_int >> 24) & 0xFF};
        if (percentage[0] == '1' && percentage[1] == '0' && percentage[2] == '0' && percentage[3] == '%') {
            std::cout << "level complete! saving and exiting\n";
            for (uint32_t j : newJumps) bestJumps.push_back(j);
            bestFitness = lastX;
            bestLastJump = lastJump;
            saveLevel(argv);
            return 0;
        }
        if (xPos > lastX && abs(lastX - globalOffset) < 0.5) std::cout << "[research] delta at beginning is " << xPos - lastX << ", offset is " << xPos - globalOffset - 5.0 << "\n";
        if (isDead) {
            std::cout << "dead (" << lastX << ", " << lastY << ") (fitness " << bestFitness << ") ("; 
            randing = false;
            lastDeathPositions.push_back((uint32_t)lastX);
            if (lastDeathPositions.size() > MAX_ATTEMPTS / 2) lastDeathPositions.pop_front();
            if (isDown) {
                jump(GD, window, false);
                isDown = false;
            }
            if (lastY >= 2700.0) {
                std::cout << (MAX_ATTEMPTS - failCount) << " tries until regression)\nnot saving this one since we fell out of bounds\n";
                if (bestJumps.size() > 0 && (signed)*bestJumps.rbegin() - (signed)xPos <= 1000 && ++failCount >= MAX_ATTEMPTS) goto regress;
            } else if (lastX > bestFitness && (lastX > bestFitnessR || failCount < 9)) {
                std::cout << (MAX_ATTEMPTS - failCount /*- (lastX <= bestFitnessR)*/) << " tries until regression)\n" << "best fitness @ " << lastX << " vs. " << bestFitness << "\n";
                bestFitness = lastX;
                if (lastX > bestFitnessR) {
                    for (uint32_t j : newJumps) bestJumps.push_back(j);
                    bestLastJump = lastJump;
                    bestFitnessR = 0;
                    failCount = 0;
                    //globalOffset = 0.0;
                    offsetDelta = 2.0;
                    saveLevel(argv);
                } else {
                    //failCount++;
                    //if (bestFitness > bestLastJump) bestFitness = bestLastJump;
                    std::cout << "not saving this one since it can be better\n";
                }
            } else if (bestJumps.empty()) {
                std::cout << (MAX_ATTEMPTS - ++failCount) << " tries until regression)\n";
            } else if (bestJumps.size() > 0 && (signed)*bestJumps.rbegin() - (signed)xPos <= 1000 && ++failCount >= MAX_ATTEMPTS) {
regress:
                std::cout << (MAX_ATTEMPTS - failCount) << " tries until regression)\n";
                /*if (jumps.size() == 0 && bestJumps.size() >= MAX_ATTEMPTS / 2) {
                    std::cerr << "forgot how to play! stopping to prevent data loss\n";
                    return 3;
                }*/
                std::cout << "too many fails, going back\n";
                bestJumps.pop_back();
                if (bestJumps.empty()) bestLastJump = 1;
                else bestLastJump = bestJumps.back();
                bestFitnessR = bestFitness;
                bestFitness = bestLastJump;
                failCount = 0;
                //globalOffset = 0.0;
                offsetDelta = 2.0;
                saveLevel(argv);
            } else if (lastDeathPositions.size() >= 5 && (/*bestFitness < 1000.0 ||*/ lastX < bestFitness - 1000.0) && std::all_of(lastDeathPositions.begin(), lastDeathPositions.end(), [lastDeathPositions](uint32_t val) -> bool {return abs((double)val - (double)lastDeathPositions.front()) < 15.0;})) { // the last 5 deaths were within 15 units of each other
                std::cout << (MAX_ATTEMPTS - failCount) << " tries until regression)\n" << "we're stuck here! ";
                /*std::list<uint32_t> lostJumps;
                for (int i = 0; i < 200; i += 5) {
                    if (bestJumps.find((uint32_t)floor(lastX / 5) * 5 - i) != bestJumps.end() && jumps.find((uint32_t)floor(lastX / 5) * 5 - i) == jumps.end()) {
                        lostJumps.push_back((uint32_t)floor(lastX / 5) * 5 - i);
                    }
                }
                // try to fix any lost jumps
                if (!lostJumps.empty()) {
                    std::cout << "looks like some jumps were lost - fixing\n";
                    // for now we'll just subtract 5 from each lost jump
                    // maybe we'll need to check to see if it needs to go up/down?
                    for (uint32_t jump : lostJumps) {
                        std::cout << "fixing lost jump at " << jump << "\n";
                        bestJumps.erase(jump);
                        bestJumps.insert(jump - 5);
                    }
                // adjust offsets if the coordinates desynced
                } else {
                    / *std::cout << "looks like the coordinates got desynced - fixing\n";
                    if (globalOffset >= offsetDelta * 8.0) {
                        if (offsetDelta <= 0.25) {
                            std::cout << "looks like it's broken for good - trying again, but manual intervention will likely be necessary\n";
                            globalOffset = 0.0;
                            offsetDelta = 2.0;
                        } else {
                            globalOffset = 0.0;
                            offsetDelta /= 2.0;
                            std::cout << "offset went over the offset boundaries, restarting with new delta " << offsetDelta << "\n";
                        }
                    } else if (globalOffset > 0.0) globalOffset = -globalOffset;
                    else globalOffset = -globalOffset + offsetDelta;
                    std::cout << "global offset is now at " << globalOffset << "\n";
                    // don't try to adjust for the next 10 attempts
                    lastDeathPositions.push_back(0);
                    lastDeathPositions.pop_front();* /
                }*/
            } else {
                std::cout << (MAX_ATTEMPTS - failCount) << " tries until regression)\n";
                //bestFitnessR = 0;
            }
            newJumps.clear();
            nextJump = bestJumps.begin();
            lastJump = 0;
            shouldJump = false;
            if (++attempt > 1000) {
                std::cout << "re-entering level to avoid breaking\nPlease Wait...\n";
                keybd_event(VK_ESCAPE, 0x01, 0, NULL);
                Sleep(50);
                keybd_event(VK_ESCAPE, 0x01, KEYEVENTF_KEYUP, NULL);
                Sleep(100);
                keybd_event(VK_ESCAPE, 0x01, 0, NULL);
                Sleep(50);
                keybd_event(VK_ESCAPE, 0x01, KEYEVENTF_KEYUP, NULL);
                Sleep(1500);
                keybd_event(VK_SPACE, 0x1c, 0, NULL);
                Sleep(50);
                keybd_event(VK_SPACE, 0x1c, KEYEVENTF_KEYUP, NULL);
                Sleep(1000);
                attempt = 0;
                globalOffset = 0.0;
                offsetDelta = 2.0;
            }
            while (isDead) mode = toMode(GD.Read<uint32_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x63C}), GD.Read<uint32_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x638}));
            float oldX = xPos;
            while (oldX == xPos) xPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x67C }) + globalOffset;
            lastX = xPos;
            continue;
        } else if (nextJump != bestJumps.end()) {
            if (xPos >= *nextJump) {
                shouldJump = true;
                nextJump++;
            }
        } else if (xPos > lastX) {
            if (!randing) {
                std::cout << "randing after " << bestLastJump << ", odds are " << (ODDS) * 100 << "%\n";
                randing = true;
            }
            double rnum = ((double)rng() / (double)rng.max());
            if (rnum < ODDS && (canJump || isDown)) shouldJump = true;
        }
        if (shouldJump) {
            if (randing) {
                newJumps.push_back(xPos);
                std::cout << "clicking!";
            } else std::cout << "clicking";
            std::cout << (modeFlying[(int)mode] || isDown ? (isDown ? " off " : " on  ") : " ") << "(" << xPos << ")\n";
            lastJump = xPos;
            if (modeFlying[(int)mode] || isDown) {
                isDown = !isDown;
                jump(GD, window, isDown);
            } else {
                jump(GD, window, true);
                if (GD.Read<uint8_t>({GD.BaseAddress, 0x3222D0, 0x164, 0x224, 0x641})) isDown = true; // handle hold ring
                else jump(GD, window, false);
            }
        }
        if (xPos > lastX) lastY = yPos;
        lastX = xPos;
    }
}
