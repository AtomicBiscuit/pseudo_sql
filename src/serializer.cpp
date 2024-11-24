#include <fstream>
#include "../include/serializer.h"

void database::serialization::save_str(std::ofstream &file, const std::string &val) {
    int buf = val.size();
    file.write(reinterpret_cast<char *>(&buf), INT_SIZE);
    file.write(val.c_str(), static_cast<int>(val.size() + 1));
}

void database::serialization::save_int(std::ofstream &file, int val) {
    file.write(reinterpret_cast<char *>(&val), INT_SIZE);
}

void database::serialization::save_bool(std::ofstream &file, bool val) {
    file.write(reinterpret_cast<char *>(&val), BOOL_SIZE);
}

void database::serialization::save_bytes(std::ofstream &file, const std::vector<bool> &val) {
    int buf = val.size() / 4;
    file.write(reinterpret_cast<char *>(&buf), INT_SIZE);
    for (int i = 0; i < val.size(); i += 4) {
        unsigned char byte_ = 8 * val[i] + 4 * val[i + 1] + 2 * val[i + 2] + val[i + 3];
        file.write(reinterpret_cast<char *>(&byte_), CHAR_SIZE);
    }
}

std::string database::serialization::load_str(std::ifstream &file) {
    int size;
    file.read(reinterpret_cast<char *>(&size), INT_SIZE);
    std::string buf(size, '\0');
    file.read(buf.data(), size + 1);
    return buf;
}

int database::serialization::load_int(std::ifstream &file) {
    int buf;
    file.read(reinterpret_cast<char *>(&buf), INT_SIZE);
    return buf;
}

bool database::serialization::load_bool(std::ifstream &file) {
    bool buf;
    file.read(reinterpret_cast<char *>(&buf), BOOL_SIZE);
    return buf;
}

std::vector<bool> database::serialization::load_bytes(std::ifstream &file) {
    int buf;
    file.read(reinterpret_cast<char *>(&buf), INT_SIZE);
    std::vector<bool> temp;
    temp.reserve(buf * 4);
    for (int i = 0; i < buf; ++i) {
        unsigned char byte_;
        file.read(reinterpret_cast<char *>(&byte_), CHAR_SIZE);
        temp.push_back(byte_ / 8);
        temp.push_back(byte_ / 4 % 2);
        temp.push_back(byte_ / 2 % 2);
        temp.push_back(byte_ % 2);
    }
    return temp;
}

