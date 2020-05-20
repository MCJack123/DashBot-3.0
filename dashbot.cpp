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
#include <algorithm>
#undef max

#define ODDS ((bestLastJump == 0.0 ? (bestFitness / 2.0) : bestLastJump) / bestFitness) / 25.0 + 0.0001

const std::unordered_map<std::string, std::vector<bool> > flyStatesForAllMaps = {
    {"StereoMadness", {false, true, false, true}},
    {"BackOnTrack", {false, true, false}},
    {"Polargeist", {false, true, false}},
    {"DryOut", {false, true, false}},
    {"BaseAfterBase", {false, true, false}},
    {"CantLetGo", {false, true, false}},
    {"Jumper", {false, true, false, true, false}},
    {"TimeMachine", {false, true, false, true}},
    {"Cycles", {false, false /*Ball*/, true, false, false}},
    {"xStep", {false, false, true, false, true, false, false, true}},
    {"Clutterfunk", {false, true, false, true, false}},
    {"TheoryOfEverything", {false, true, false, false /*UFO*/, false, false, true, false, false, false, false, true}},
    {"ElectromanAdventures", {false, true, false, false, false, false, true, false, true, false, false, false}},
    {"Clubstep", {false, true, false, false, false, true, false, true, false, true, false, true, false, true}},
    {"Electrodynamix", {false, true, false, false, false, false, true, false, true, false, false, true}},
    {"HexagonForce", {false, true, false, false, true, false, false, false, true, false, true}},
    {"BlastProcessing", {false, true /*Wave*/, true, false, true, false, false, true, false, false, true, false}},
    {"TheoryOfEverything2", {false, true, false, false, false, false, true, false, true, false, true, true, false, true, false, false, true, false}},
    {"GeometricalDominator", {false, true /*Robot?*/, true, true, false, true, true, false, false, true, false, false, true, true}},
    //{"Deadlocked", {}},
    // Fingerdash is currently impossible due to the hold jump rings
};

std::vector<bool> flyStatesForMap;
double bestFitness = 1;
double bestFitnessR = 0;
std::set<uint32_t> bestJumps;
double bestLastJump = 1;
int failCount = 0;
float lastX = 0;
std::ofstream out;
bool saving = false;
uint32_t oldFlyState = 0;
int currentFlyNum = 0;
bool isDown = false;

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
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <level name|portal types> [save.dbj]\nType \"" << argv[0] << " list\" to list level names\n";
        return 1;
    } else if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?") {
        std::cout << "DashBot is a simple AI designed to beat Geometry Dash levels. This version works with Geometry Dash 2.11.\nUsage: " << argv[0] << " <level name|portal types> [save.dbj]\nType \"" << argv[0] << " list\" to list level names.\n";
        return 0;
    } else if (std::string(argv[1]) == "list") {
        std::cout << "List of known level names: ";
        for (std::pair<std::string, std::vector<bool> > p : flyStatesForAllMaps) std::cout << p.first << ", ";
        std::cout << "\nTo use DashBot with another level, you must supply the portal type list yourself. The string must be a binary string specifying whether the bot should tap (0: cube, ball, UFO, spider) or hold (1: ship, wave, robot?) the mouse. Each digit represents a region between two of either the start of the map, the end of the map, or a portal. For example, Stereo Madness's portal string would be '0101' since it's mode sequence is cube, ship, cube, ship.\n";
        return 0;
    } else if (flyStatesForAllMaps.find(argv[1]) != flyStatesForAllMaps.end()) 
        flyStatesForMap = flyStatesForAllMaps.at(argv[1]);
    else if (std::all_of(argv[1], argv[1] + strlen(argv[1]), [](char c)->bool{return c == '0' || c == '1';})) 
        for (int i = 0; i < strlen(argv[1]); i++) flyStatesForMap.push_back(argv[1][i] == '1');
    else {
        std::cerr << "Usage: " << argv[0] << " <level name|portal types> [save.dbj]\nType \"" << argv[0] << " list\" to list level names\n";
        return 1;
    }
    std::cout << "DashBot v3.0\nBased on Pizzabot-v4 by Pizzaroot\n";
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
                bestJumps.insert((n / 5) * 5);
            }
            in.close();
        } else std::cerr << "Could not open input file, ignore this if you're creating a new save.";
    }
    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());
    HackIH GD;
    GD.bind("GeometryDash.exe");
    std::set<uint32_t> jumps;
    double lastJump = 0;
    bool randing = false;
    oldFlyState = GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x534 });
    while (true) {
        bool shouldJump = false;
        float xPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x67C });
        float yPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x38C, 0xB4, 0x224, 0x680 });
        uint32_t flyState = xPos != lastX ? GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164 , 0x224 , 0x534 }) : oldFlyState;
        char percentage0 = GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12C });
        if ((percentage0 == '1' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12D }) == '0' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12E }) == '0') || (percentage0 == '9' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12D }) == '9')) {
            std::cout << "level complete! saving and exiting\n";
            bestFitness = lastX;
            bestJumps = jumps;
            bestLastJump = lastJump;
            saveLevel(argv);
            return 0;
        }
        if (flyState != oldFlyState && xPos > lastX) {
            std::cout << "switched between modes\n";
            currentFlyNum++;
            oldFlyState = flyState;
            if (isDown) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                isDown = false;
            }
        }
        if (xPos < lastX && lastX - xPos > 10) {
            std::cout << "dead (" << lastX << ") (";
            randing = false;
            oldFlyState = flyState;
            currentFlyNum = 0;
            if (isDown) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                isDown = false;
            }
            if (lastX > bestFitness && (lastX > bestFitnessR || failCount < 9)) {
                std::cout << (10 - failCount - (lastX <= bestFitnessR)) << " tries until regression)\n" << "best fitness @ " << lastX << " vs. " << bestFitness << "\n";
                bestFitness = lastX;
                if (lastX > bestFitnessR) {
                    bestJumps = jumps;
                    bestLastJump = lastJump;
                    bestFitnessR = 0;
                    saveLevel(argv);
                    failCount = 0;
                } else {
                    failCount++;
                    std::cout << "not saving this one since it can be better\n";
                }
            } else if ((signed)bestJumps.size() - (signed)jumps.size() <= 10 && ++failCount >= 10 && bestJumps.size() > 0) {
                std::cout << (10 - failCount) << " tries until regression)\n";
                if (jumps.size() == 0 && bestJumps.size() >= 5) {
                    std::cerr << "forgot how to play! stopping to prevent data loss\n";
                    return 3;
                }
                std::cout << "too many fails, going back\n";
                uint32_t max = 0, omax = 0;
                for (uint32_t n : bestJumps) if (n > max) {omax = max; max = n;}
                bestJumps.erase(max);
                bestLastJump = omax;
                bestFitnessR = bestFitness;
                bestFitness = bestLastJump;
                failCount = 0;
                saveLevel(argv);
            } else {
                std::cout << (10 - failCount) << " tries until regression)\n";
                //bestFitnessR = 0;
            }
            jumps.clear();
            lastJump = 0;
        } else if ((uint32_t)floor(xPos / 5) * 5 <= bestLastJump) {
            if (bestJumps.find((uint32_t)floor(xPos / 5) * 5) != bestJumps.end() && jumps.find((uint32_t)floor(xPos / 5) * 5) == jumps.end())
                shouldJump = true;
        } else if (xPos > lastX) {
            if (!randing) {
                std::cout << "randing after " << bestLastJump << ", odds are " << (ODDS) * 100 << "%\n";
                randing = true;
            }
            double rnum = ((double)rng() / (double)rng.max());
            if (rnum < ODDS) shouldJump = true;
        }
        if (shouldJump) {
            std::cout << "clicking (" << (uint32_t)floor(xPos / 5) * 5 << ")\n";
            jumps.insert((uint32_t)floor(xPos / 5) * 5);
            lastJump = floor(xPos / 5) * 5;
            if (flyStatesForMap[currentFlyNum]) {
                mouse_event(isDown ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                isDown = !isDown;
            } else {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                Sleep(75);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }
        }
        lastX = xPos;
    }
}