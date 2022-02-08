#include "cartridge.h"
#include "log.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb/stb_image.h>
#include <sstream>
#include <map>
#include <algorithm>
#include "events.h"
#include "http.h"

std::string p8lua_to_std_lua(std::string& s);
std::string decompress_lua(std::vector<unsigned char> &compressed_lua);

Cartridge* load_from_memory(unsigned char* data, int width, int height) {
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

Cartridge* load_from_png(std::string path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (data == NULL) {
        logger << "Couldn't open file " << path << ": " << stbi_failure_reason() << ENDL;
        return NULL;
    }

    return load_from_memory(data, width, height);
}

Cartridge* load_from_url(std::string url)
{
    int width, height, channels;
    std::vector<unsigned char> png_data = http_get(url);
    unsigned char* data = stbi_load_from_memory(&png_data[0], png_data.size(), &width, &height, &channels, STBI_rgb_alpha);

    if (data == NULL) {
        logger << "Couldn't load from url " << url << ": " << stbi_failure_reason() << ENDL;
        return NULL;
    }

    return load_from_memory(data, width, height);
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

                if (length > decompressed_length - d_i) {
                    logger << "Error when decompressing: it would overflow the buffer" << ENDL;
                    break;
                }
                if (start >= d_i) {
                    logger << "Error when decompressing: reading from uninitialized data" << ENDL;
                    break;
                }

                for (int i = 0; i < length; i++) {
                    decompressed[d_i++] = decompressed[start + i];
                }
            }
        }
        return std::string(decompressed.begin(), decompressed.end());
    }
    if (compressed_lua[0] == ':' && compressed_lua[1] == 'c' && compressed_lua[2] == ':' && compressed_lua[3] == 0) {
        std::vector<unsigned char> header(8);
        for (int i = 0; i < header.size(); i++) {
            header[i] = br.next_u8(8);
        }
        unsigned int decompressed_length = (header[4] << 8) | header[5];
        std::vector<unsigned char> decompressed(decompressed_length);
        unsigned int d_i = 0;

        std::string lut = "\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";
        while (d_i < decompressed_length) {
            unsigned char byte = br.next_u8(8);
            if (byte == 0) {
                decompressed[d_i++] = br.next_u8(8);
            }
            else if (byte < 0x3c) {
                decompressed[d_i++] = lut[byte-1];
            }
            else {
                unsigned char next = br.next_u8(8);
                unsigned int offset = (byte - 0x3c) * 16 + (next & 0x0f);
                unsigned int length = (next >> 4) + 2;
                unsigned int start = d_i - offset;
                for (int i = 0; i < length; i++) {
                    decompressed[d_i++] = decompressed[start + i];
                }
            }
        }
        return std::string(decompressed.begin(), decompressed.end());
    }

    return std::string(compressed_lua.begin(), compressed_lua.end());
}

#define SPECIAL_CHAR_OFFSET 0x7E

char command_chars[] = {
    '*', '#', '-', '|', '+', '^'
};

#include <algorithm>
std::string replace_escape_chars(std::string& line) {
    // Biggest conflict I can think of is print(200\103) prints 1 (integer division), print("200\103") prints 200G (escaped character)
    // I'll *try* to solve this by tracking if we're inside a string or not.
    // In fact, P8SCII only applies when we're inside a string! `asdf=123 \n print(200\asdf)` is also an integer division. `\a` is also a P8SCII control character, but because it's not inside a string it doesn't apply
    // Hopefully keeping track of quote marks will be enough... TODO print("123\"456")
    unsigned char string_char = 0;

    for (int i = 0; i < line.size(); i++) {
        if (line[i] == '"' || line[i] == '\'') {
            if (string_char == line[i]) {
                string_char = 0;
                continue;
            }
            if (string_char == 0) {
                string_char = line[i];
                continue;
            }
        }

        if (line[i] == '\\') {
            if (string_char == 0) {
                // Assume integer division
                line.replace(i, 1, "//");
            } else {
                // We don't need to replace special characters e.g. \126, lua already parses them correctly
                char next = line[i + 1];
                if (next == '\\') {
                    line.replace(i, 2, "\\\\/" + next);
                } else {
                    for (int j = 0; j < 6; j++) {
                        if (command_chars[j] == next) {
                            line.replace(i, 1, "\\\\");
                            i += 2;
                        }
                    }
                }
            }
        }
    }

    return line;
}

std::string p8lua_to_std_lua(std::string& s) {
    std::ostringstream out;
    std::istringstream in(s);

    std::string line;
    int line_num = 1;
    while (std::getline(in, line)) {
        int pos;

        // Remove trailing \0
        int nilchar = line.find(std::string("\0", 1));
        if (nilchar != std::string::npos) {
            line = line.substr(0, nilchar);
        }

        line = replace_escape_chars(line);

        if (((pos = line.find("for")) != std::string::npos) &&
            ((pos = line.find("do", pos)) != std::string::npos) &&
            line[pos - 1] >= '0' && line[pos - 1] <= '9') {
            line = line.replace(pos, 0, " ");
        }

        out << line << ENDL;
        line_num++;
    }

    return out.str();
}