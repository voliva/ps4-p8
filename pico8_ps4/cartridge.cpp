#include "cartridge.h"
#include "log.h"

#ifndef __PS4__ // OpenOrbis already has these ones precompiled
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <stb/stb_image.h>
#include <sstream>
#include <map>
#include "events.h"

std::string p8lua_to_std_lua(std::string& s);
std::string decompress_lua(std::vector<unsigned char> &compressed_lua);

Cartridge *load_from_png(std::string path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (data == NULL) {
        logger << "Couldn't open file " << path << ": " << stbi_failure_reason() << ENDL;
        return NULL;
    }

    std::vector<unsigned char> p8_bytes(width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char result = 0;
            result = (result << 2) | (0x03 & data[(y * width + x) * 4 + 3]); // a
            result = (result << 2) | (0x03 & data[(y * width + x) * 4 + 0]); // r
            result = (result << 2) | (0x03 & data[(y * width + x) * 4 + 1]); // g
            result = (result << 2) | (0x03 & data[(y * width + x) * 4 + 2]); // b
            p8_bytes[y * width + x] = result;
        }
    }

    stbi_image_free(data);

    Cartridge* ret = new Cartridge;
    ret->sprite_map = std::vector<unsigned char>(p8_bytes.begin() + 0, p8_bytes.begin() + 0x3000);
    ret->sprite_flags = std::vector<unsigned char>(p8_bytes.begin() + 0x3000, p8_bytes.begin() + 0x3100);
    ret->music = std::vector<unsigned char>(p8_bytes.begin() + 0x3100, p8_bytes.begin() + 0x3200);
    ret->sfx = std::vector<unsigned char>(p8_bytes.begin() + 0x3200, p8_bytes.begin() + 0x4300);

    std::vector<unsigned char> compressed_lua(p8_bytes.begin() + 0x4300, p8_bytes.begin() + 0x7FFF);
    std::string p8_lua = decompress_lua(compressed_lua);

    ret->lua = p8lua_to_std_lua(p8_lua);

    return ret;
}

class BinaryReader {
public:
    BinaryReader(std::vector<unsigned char>* buffer) {
        this->buffer = buffer;
    }
    unsigned char next_bit() {
        unsigned char v = (*this->buffer)[this->pointer];
        unsigned char ret = (v >> (7 - this->bit)) & 0x01;
        if (this->bit == 0) {
            this->bit = 7;
            this->pointer += 1;
        }
        else {
            this->bit -= 1;
        }

        return ret;
    }
    unsigned char next_u8(int n) {
        unsigned char v = 0;
        for (int i = 0; i < n; i++) {
            if (this->next_bit() == 1) {
                v |= 1 << i;
            }
        }
        return v;
    }
    unsigned int next_int(int n) {
        unsigned int v = 0;
        for (int i = 0; i < n; i++) {
            if (this->next_bit() == 1) {
                v |= 1 << i;
            }
        }
        return v;
    }

private:
    std::vector<unsigned char>* buffer;
    int pointer = 0;
    int bit = 7;
};

typedef struct {
    unsigned char value;
    int next;
} MtfNode;

class MoveToFront {
public:
    MoveToFront() : nodes(256) {
        for (int i = 0; i < 256; i++) {
            this->nodes[i].value = i;
            this->nodes[i].next = i + 1;
        }
    }
    unsigned char getAndMove(int index) {
        if (index == 0) {
            return this->nodes[this->root].value;
        }

        int p = this->root;
        int prev = -1;
        for (int i = 0; i < index; i++) {
            prev = p;
            p = this->nodes[p].next;
        }

        this->nodes[prev].next = this->nodes[p].next;
        this->nodes[p].next = this->root;
        this->root = p;
        return this->nodes[p].value;
    }

private:
    std::vector<MtfNode> nodes;
    int root = 0;
};

std::string decompress_lua(std::vector<unsigned char> &compressed_lua) {
    BinaryReader br(&compressed_lua);

    if (compressed_lua[0] == 0 && compressed_lua[1] == 'p' && compressed_lua[2] == 'x' && compressed_lua[3] == 'a') {
        std::vector<unsigned char> header(8);
        for (int i = 0; i < header.size(); i++) {
            header[i] = br.next_u8(8);
        }
        unsigned int decompressed_length = (header[4] << 8) | header[5];
        std::vector<unsigned char> decompressed(decompressed_length);
        unsigned int d_i = 0;

        MoveToFront mtf;
        while (d_i < decompressed_length) {
            unsigned char type = br.next_bit();
            if (type == 1) {
                unsigned char unary = 0;
                while (br.next_bit() == 1) {
                    unary++;
                }
                unsigned char unary_mask = (1 << unary) - 1;
                unsigned char index = br.next_u8(4 + unary) + (unary_mask << 4);
                decompressed[d_i++] = mtf.getAndMove(index);
            } else {
                int offset_bits;
                if (br.next_bit() == 1) {
                    if (br.next_bit() == 1) {
                        offset_bits = 5;
                    }
                    else {
                        offset_bits = 10;
                    }
                }
                else {
                    offset_bits = 15;
                }

                unsigned int offset = br.next_int(offset_bits) + 1;
                unsigned int length = 3;
                unsigned char part = 0;
                do {
                    part = br.next_u8(3);
                    length += part;
                } while (part == 7);
                unsigned int start = d_i - offset;
                for (int i = 0; i < length; i++) {
                    decompressed[d_i++] = decompressed[start + i];
                }
            }
        }
        return std::string(decompressed.begin(), decompressed.end());
    }
    if (compressed_lua[0] == ':' && compressed_lua[1] == 'c' && compressed_lua[1] == ':' && compressed_lua[1] == 0) {
        // TODO old compression
    }

    return std::string(compressed_lua.begin(), compressed_lua.end());
}

// TODO https://www.lexaloffle.com/bbs/?tid=3739
std::map<unsigned char, std::string> special_char_replacement = {
    {126, "~"},
    // Missing nubbin
    {128, "█"},
    {129, "▒"},
    {130, "🐱"},
    {131, "⬇️"},
    {132, "░"},
    {133, "✽"},
    {134, "●"},
    {135, "♥"},
    {136, "☉"},
    {137, "웃"},
    {138, "⌂"},
    {139, "⬅️"},
    {140, "😐"},
    {141, "♪"},
    {142, "🅾️"},
    {143, "◆"},
    {144, "…"},
    {145, "➡️"},
    // Missing star
    {147, "⧗"},
    {148, "⬆️"},
    {149, "ˇ"},
    {150, "∧"},
    {151, "❎"},
    {152, "▤"},
    // Missing vertical lines
};
std::string replace_special_chars(std::string& line) {
    /*for (std::map<unsigned char, std::string>::iterator iter = special_char_replacement.begin(); iter != special_char_replacement.end(); iter++)
    {
        unsigned char k = iter->first;
    }*/
    for (const auto& myPair : special_char_replacement) {
        unsigned char key = myPair.first;
        int pos = line.find(key);
        if (pos != std::string::npos) {
            line.replace(pos, 1, special_char_replacement[key]);
        }
    }
    return line;
}


std::map<std::string, P8_Key> button_to_key = {
    {"⬅️", P8_Key::left},
    {"➡️", P8_Key::right},
    {"⬆️", P8_Key::up},
    {"⬇️", P8_Key::down},
    {"🅾️", P8_Key::circle},
    {"❎", P8_Key::cross},
};

const std::string WHITESPACE = " \n\r\t\f\v";
std::string p8lua_to_std_lua(std::string& s) {
    std::ostringstream out;
    std::istringstream in(s);

    std::string line;
    while (std::getline(in, line)) {
        int pos;

        // special chars
        line = replace_special_chars(line);

        if (line.find("btn(") != std::string::npos || line.find("btnp(") != std::string::npos) {
            bool replaced = true;
            while (replaced) {
                replaced = false;
                for (const auto& myPair : button_to_key) {
                    std::string key = myPair.first;
                    if ((pos = line.find(key)) != std::string::npos) {
                        line = line.replace(pos, key.length(), std::to_string((int)button_to_key[key]));
                        replaced = true;
                    }
                }
            }
        }

        // TODO fillp patterns
        if (line.find("fillp(") != std::string::npos) {
            line = "";
        }

        // ?"..." => print("...")
        pos = line.find_first_not_of(WHITESPACE);
        if (pos != std::string::npos && line[pos] == '?') {
            line = line.substr(0, pos) + "print(" + line.substr(pos +1) + ")";
        }

        // != => ~=
        pos = 0;
        while ((pos = line.find("!=", pos)) != std::string::npos) {
            line.replace(pos, 2, "~=");
            pos += 2;
        }

        // \- => \\-
        pos = 0;
        while ((pos = line.find("\\-", pos)) != std::string::npos) {
            line.replace(pos, 2, "\\\\-");
            pos += 2;
        }

        // \ => // (integer division)
        pos = 0;
        while ((pos = line.find("\\", pos)) != std::string::npos) {
            // Skip escaped characters
            if (line[pos + 1] != '\\') {
                line.replace(pos, 1, "//");
            }
            pos += 2;
        }

        // This is all so very hacky... but I don't want to write a custom lua (yet) if I can get away with it
        // binary literals
        pos = 0;
        while ((pos = line.find("0b", pos)) != std::string::npos) {
            int p = pos + 2;
            int number = 0;
            while (p < line.length() && (line[p] == '0' || line[p] == '1')) {
                number = number << 1;
                if (line[p] == '1') {
                    number = number | 1;
                }
                p++;
            }

            line.replace(pos, p-pos, std::to_string(number));
            pos += 2;
        }

        if ((pos = line.find("-=")) != std::string::npos) {
            int last_word_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            int last_word_start = line.substr(0, last_word_end).find_last_of(WHITESPACE);

            line = line.substr(0, pos) + "=" + line.substr(last_word_start, pos - last_word_start) + "-" + line.substr(pos+2);
        }
        if ((pos = line.find("+=")) != std::string::npos) {
            int last_word_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            int last_word_start = line.substr(0, last_word_end).find_last_of(WHITESPACE);

            line = line.substr(0, pos) + "=" + line.substr(last_word_start, pos - last_word_start) + "+" + line.substr(pos + 2);
        }
        if ((pos = line.find("/=")) != std::string::npos) {
            int last_word_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            int last_word_start = line.substr(0, last_word_end).find_last_of(WHITESPACE);

            line = line.substr(0, pos) + "=" + line.substr(last_word_start, pos - last_word_start) + "/" + line.substr(pos + 2);
        }
        if ((pos = line.find("*=")) != std::string::npos) {
            int last_word_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            int last_word_start = line.substr(0, last_word_end).find_last_of(WHITESPACE);

            line = line.substr(0, pos) + "=" + line.substr(last_word_start, pos - last_word_start) + "*" + line.substr(pos + 2);
        }

        out << line << ENDL;
    }

    return out.str();
}