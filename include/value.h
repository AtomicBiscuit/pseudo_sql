#pragma once

#include <string>
#include <variant>
#include <vector>
#include <sstream>

namespace database {
    enum class Type {
        Integer = 0, Boolean, String, Bytes, None,
    };

    static inline std::string type_to_str(Type t) {
        switch (t) {
            case (Type::Integer):
                return "int32";
            case (Type::Boolean):
                return "bool";
            case (Type::String):
                return "string";
            case (Type::Bytes):
                return "bytes";
            default:
                return "none";
        }
    }

    using value_t = std::variant<int, bool, std::string, std::vector<bool>>;

    static inline std::string value_to_string(const value_t &val, Type type) {
        if (type == database::Type::Integer)
            return std::to_string(get<int>(val));
        if (type == database::Type::Boolean)
            return std::to_string(get<bool>(val));
        if (type == database::Type::String)
            return get<std::string>(val);
        std::stringstream out;
        out << "0x";
        const auto &bytes = get<std::vector<bool >>(val);
        for (size_t i = 0; i < bytes.size(); i += 4) {
            out << std::hex << 8 * bytes[i] + 4 * bytes[i + 1] + 2 * bytes[i + 2] + bytes[i + 3];
        }
        return out.str();
    }
}