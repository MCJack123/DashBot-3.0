#include "HAPIH.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <set>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

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
float lastX = 0;
uint32_t oldFlyState = 0;
int currentFlyNum = 0;
bool isDown = false;

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <save.dbj>\n";
        return 1;
    } else if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h" || std::string(argv[1]) == "-?") {
        std::cout << "DashBot is a simple AI designed to beat Geometry Dash levels. This version works with Geometry Dash 2.11.\nUsage: " << argv[0] << " <save.dbj>\n";
        return 0;
    }
    std::cout << "DashBot Player v3.0\nOffsets from Pizzabot-v4 by Pizzaroot\n";
    std::ifstream in(argv[1], std::ios_base::binary);
    if (in.is_open()) {
        in.read((char*)&bestFitness, sizeof(double));
        in.read((char*)&bestLastJump, sizeof(double));
        std::string type;
        char c;
        while ((c = in.get())) type += c;
        uint32_t n = 0;
        while (!in.eof()) {
            in.read((char*)&n, 4);
            if (!n) break;
            bestJumps.insert((n / 5) * 5);
        }
        in.close();
        if (type.find(':')) type.erase(type.find(':'));
        if (flyStatesForAllMaps.find(type) != flyStatesForAllMaps.end()) 
            flyStatesForMap = flyStatesForAllMaps.at(type);
        else if (std::all_of(type.begin(), type.end(), [](char c)->bool{return c == '0' || c == '1';})) 
            for (int i = 0; i < type.size(); i++) flyStatesForMap.push_back(type[i] == '1');
        else {
            std::cerr << "Unknown level type " << type << " in save file\n";
            return 1;
        }
    } else {
        std::cerr << "Could not open " << argv[1] << "for reading\n";
        return 2;
    }
    HackIH GD;
    //GD.SetDebugOutput(std::cerr);
    GD.bind("GeometryDash.exe");
    std::set<uint32_t> jumps;
    oldFlyState = GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x534 });
    while (true) {
        float xPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x224, 0x67C });
        float yPos = GD.Read<float>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x38C, 0xB4, 0x224, 0x680 });
        uint32_t flyState = xPos != lastX ? GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164 , 0x224 , 0x534 }) : oldFlyState;
        char percentage0 = GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12C });
        if ((percentage0 == '1' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12D }) == '0' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12E }) == '0') || (percentage0 == '9' && GD.Read<char>({ GD.BaseAddress , 0x3222D0 , 0x164, 0x404, 0xB4, 0x3C0, 0xE8, 0x8, 0x12D }) == '9')) {
            std::cout << "level complete! exiting\n";
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
        if (xPos < lastX) {
            std::cout << "died?!? (" << lastX << ")\n";
            flyState = GD.Read<uint32_t>({ GD.BaseAddress , 0x3222D0 , 0x164 , 0x224 , 0x534 });
            currentFlyNum = 0;
            if (isDown) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                isDown = false;
            }
            jumps.clear();
        } else if ((uint32_t)floor(xPos / 5) * 5 <= bestLastJump) {
            if (bestJumps.find((uint32_t)floor(xPos / 5) * 5) != bestJumps.end() && jumps.find((uint32_t)floor(xPos / 5) * 5) == jumps.end()) {
                std::cout << "clicking (" << (uint32_t)floor(xPos / 5) * 5 << ")\n";
                jumps.insert((uint32_t)floor(xPos / 5) * 5);
                if (flyStatesForMap[currentFlyNum]) {
                    mouse_event(isDown ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    isDown = !isDown;
                } else {
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    Sleep(75);
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                }
            }
        }
        lastX = xPos;
    }
}
