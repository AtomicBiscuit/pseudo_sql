#pragma once

#include "value.h"

#include <string>

namespace database {
    namespace serialization {
        constexpr unsigned CHAR_SIZE = sizeof(char);
        constexpr unsigned INT_SIZE = sizeof(int) / CHAR_SIZE;
        constexpr unsigned BOOL_SIZE = sizeof(bool) / CHAR_SIZE;
        constexpr unsigned BYTE_SIZE = sizeof(std::byte) / CHAR_SIZE;

        void save_str(std::ofstream &, const std::string &);

        void save_int(std::ofstream &, int);

        void save_bool(std::ofstream &, bool);

        void save_bytes(std::ofstream &, const std::vector<bool> &);


        template<typename T>
        requires(std::convertible_to<T, value_t>)
        inline void save(std::ofstream &, const T &) {};

        template<>
        inline void save(std::ofstream &file, const int &val) { save_int(file, val); }

        template<>
        inline void save(std::ofstream &file, const std::string &val) { save_str(file, val); }

        template<>
        inline void save(std::ofstream &file, const bool &val) { save_bool(file, val); }

        template<>
        inline void save(std::ofstream &file, const std::vector<bool> &val) { save_bytes(file, val); }

        std::string load_str(std::ifstream &);

        int load_int(std::ifstream &);

        bool load_bool(std::ifstream &);

        std::vector<bool> load_bytes(std::ifstream &);

        template<typename T>
        requires(std::convertible_to<T, value_t>)
        inline T load(std::ifstream &) {};

        template<>
        inline int load(std::ifstream &file) { return load_int(file); }

        template<>
        inline std::string load(std::ifstream &file) { return load_str(file); }

        template<>
        inline bool load(std::ifstream &file) { return load_bool(file); }

        template<>
        inline std::vector<bool> load(std::ifstream &file) { return load_bytes(file); }
    }
}