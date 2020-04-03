#include <iostream>
#include <fstream>
#include <string>

int main(int argc, const char * argv[]) {
    std::ofstream out(argv[1], std::ios_base::binary);
    double a = 0;
    std::cin >> a;
    std::cin.ignore(100, '\n');
    out.write((char*)&a, sizeof(double));
    std::cin >> a;
    std::cin.ignore(100, '\n');
    out.write((char*)&a, sizeof(double));
    std::string type;
    std::getline(std::cin, type);
    out.write(type.c_str(), type.size() + 1);
    uint32_t n = 0;
    do {
        std::cin >> n;
        std::cin.ignore(100, '\n');
        out.write((char*)&n, 4);
    } while (n);
    out.close();
    return 0;
}