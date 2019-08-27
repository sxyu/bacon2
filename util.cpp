#include "util.hpp"

#include <iostream>
#include "tinydir.h"

namespace {
    // Convert a 16-int integer to 4 hex chars
    void to_hex(uint16_t w, std::string& output_hex) {
        static constexpr char digits[] = "0123456789ABCDEF";
        for (int j=12; j >= 0; j-=4)
            output_hex.push_back(digits[(w>>j) & 0x0f]);
    }

    // Convert back to int
    uint16_t to_dec(const std::string& hex, int pos) {
        static constexpr int mult[] = {16*16*16, 16*16, 16, 1};
        uint16_t result = 0;
        for (int i = 0; i < 4; ++i) {
            if (static_cast<size_t>(pos + i) >= hex.size()) return result;
            char c = hex[pos+i];
            if (c <= '9') {
                result += mult[i] * static_cast<int>(c - '0');
            } else {
                result += mult[i] * (static_cast<int>(c - 'A') + 10);
            }
        }
        return result;
    }
}

namespace bacon {
namespace util {

std::string trim_name(const std::string& name, int max_len) {
    if (name.size() > 40) {
        return name.substr(0, 40) + "...";
    }
    return name;
}

std::string quote(const std::string& string) {
    std::string sb("\"");
    sb.reserve(string.size() + 2);
    for (auto c : string) {
        switch (c) {
            case '\\':
            case '"':
                sb.push_back('\\');
                sb.push_back(c);
                break;
            case '\b': sb.append("\\b"); break;
            case '\t': sb.append("\\t"); break;
            case '\n': sb.append("\\n"); break;
            case '\f': sb.append("\\f"); break;
            case '\r': sb.append("\\r"); break;
            default:
                if (c < ' ') {
                    sb.append("\\u");
                    to_hex(static_cast<uint16_t>(c), sb);
                } else {
                    sb.push_back(c);
                }
        }
    }
    sb.push_back('"');
    return sb;
}

std::string unquote(const std::string& string) {
    if (string.empty() || string.front() != '"' || string.back() != '"') {
        // already unquoted?
        return string;
    }
    std::string sb;
    sb.reserve(string.size() - 2);
    for (size_t i = 1; i < string.size()-1; ++i) {
        if (string[i] == '\\') {
            switch (string[i+1]) {
                case 'u':
                    sb.push_back(static_cast<char>(to_dec(string, i+1)));
                    i += 4;
                    break;
                case 'b': sb.push_back('\b'); break;
                case 't': sb.push_back('\t'); break;
                case 'n': sb.push_back('\n'); break;
                case 'f': sb.push_back('\f'); break;
                case 'r': sb.push_back('\r'); break;
                default:
                  // includes '"\ etc
                  sb.push_back(string[i+1]);
            }
            ++i;
        } else {
            sb.push_back(string[i]);
        }
    }
    return sb;
}

std::vector<std::string> lsdir(const std::string & path) {
    tinydir_dir dir;
    tinydir_open(&dir, path.c_str());

    std::vector<std::string> out;
    while (dir.has_next) {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        std::string fname(file.name);
        if (fname != "." && fname != "..") {
            out.emplace_back(fname);
        }
        tinydir_next(&dir);
    }
    return out;
}

void create_dir(const std::string& path) {
    #ifdef _WIN32
        // Windows only
        CreateDirectoryA(path.c_str(), NULL);
    #else
        // make the storage directory, if possible
        if (system((std::string("mkdir -p ") + path).c_str())) {
            throw std::runtime_error("Bacon internal error: failed to create directory");
        }
    #endif    
}

void remove_dir(const std::string& path) {
    #ifdef _WIN32
        // Windows only
        RemoveDirectoryA(path.c_str());
    #else
        // make the storage directory, if possible
        if (system((std::string("rm -rf ") + path).c_str())) {
            throw std::runtime_error("Bacon internal error: failed to remove directory");
        }
    #endif    
}
}
}
