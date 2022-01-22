#include "cartridge.h"
#include "log.h"

#ifndef __PS4__ // OpenOrbis already has these ones precompiled
#define STB_IMAGE_IMPLEMENTATION
#endif

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

std::map<unsigned char, P8_Key> button_to_key = {
    {139, P8_Key::left},
    {145, P8_Key::right},
    {148, P8_Key::up},
    {131, P8_Key::down},
    {142, P8_Key::circle},
    {151, P8_Key::cross},
};

const std::string WHITESPACE = " \n\r\t\f\v";
const std::string WHITESPACE_CLOSEPARENS = " \n\r\t\f\v)";
const std::string ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define AO_LENGTH 16
const std::string assignmentOperators[] = { "-", "+", "/", "*", "..", "\\", "%", "^", "|", "&", "^^", ">>>", "<<>", ">><", "<<", ">>"};

bool is_alphanumeric(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9');
}

int find_end_of_statement(std::string s) {
    int pos = s.find(";");
    if (pos != std::string::npos) {
        return pos;
    }
        
    pos = s.find_first_not_of(WHITESPACE);
    if (pos == std::string::npos) {
        return -1;
    }

    bool found_operator;
    do {
        // case `x += shr(12)`: We can't look straight for the next whitespace, we need to follow the brackets instead
        int parens_pos = s.find_first_of("(", pos);
        int whitespace_pos = s.find_first_of(WHITESPACE, pos);

        if (parens_pos != std::string::npos && (whitespace_pos == std::string::npos || parens_pos < whitespace_pos)) {
            pos = parens_pos;
            int parens = 1;
            while (parens > 0 && pos < s.length()-1) {
                pos++;
                if (s[pos] == '(') {
                    parens++;
                }
                else if (s[pos] == ')') {
                    parens--;
                }
            }
            if (parens > 0) {
                return -1;
            }
            pos++;
        }
        else {
            pos = whitespace_pos;
        }

        pos = s.find_first_not_of(WHITESPACE, pos);
        if (pos == std::string::npos) {
            return s.length();
        }
        found_operator = false;
        for (int i = 0; i < AO_LENGTH; i++) {
            if (s[pos] == assignmentOperators[i][0]) {
                found_operator = true;
            }
        }
        if (found_operator) {
            // Advance to next token
            pos = s.find_first_not_of(WHITESPACE, pos + 1);
        }
    } while (found_operator);
    return pos;
}

std::string replace_assignment_operators(std::string& line) {
    for (int i = 0; i < AO_LENGTH; i++) {
        std::string op = assignmentOperators[i];
        std::string assignment = op + "=";

        /*
        * We can't just unroll x -= k to x = x - k, because this case:
        * value -= 3 + 2
        * would become `value = value - 3 + 2`, but it should become `value = value - (3 + 2)` instead.
        * But we can't put parenthesis on "the rest of the line", because this is also valid lua:
        * x -= 2     x -= 3
        * x -= 2 + y x -= 3
        * One idea I had is to break it down into many statements, adding in a temporary variable.
        * So `value -= 3 + 2` becomes `_tmp0 = 3 + 2 value = value + _tmp0`.
        * This would also work in multi-statement:
        * `x -= 2     x -= 3` => `_tmp0 = 2 x -= 3 x = x - _tmp0` => `_tmp0 = 2 _tmp1 = 3 x = x - _tmp0 x = x - _tmp1`
        * But this will break if the order of execution is important, and will break with things like `if something then x += 2 end`
        *
        * So I'll do another idea: Put parenthesis as long as we don't find any other operator. Let's build a parser yay!
        */

        int pos = 0;
        while ((pos = line.find(assignment)) != std::string::npos) {
            int last_word_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            // CLOSEPARENS case: if(condition)x+=3
            int last_word_start = line.substr(0, last_word_end).find_last_of(WHITESPACE_CLOSEPARENS) + 1;

            std::string code_before_assignment = line.substr(0, pos);
            std::string variable_name = line.substr(last_word_start, last_word_end - last_word_start + 1);
            std::string code_after_assignment = line.substr(pos + assignment.size());

            pos = find_end_of_statement(code_after_assignment);

            if (pos == -1) { // Line ends before the statement finishes => I guess don't worry about it?
                line = code_before_assignment + "=" + variable_name + op + code_after_assignment;
            }
            else {
                std::string assignment_expression = code_after_assignment.substr(0, pos);
                std::string other_statements = code_after_assignment.substr(pos);

                line = code_before_assignment + "=" + variable_name + op + "(" + assignment_expression + ") " + other_statements;
            }
        }
    }
    return line;
}

std::map<std::string, std::string> binary_functions = {
    {"|", "bor"},
    {"&", "band"},
    {"^^", "bxor"},
    {">>>", "lshr"},
    {"<<>", "rotl"},
    {">><", "rotr"},
    {"<<", "shl"},
    {">>", "shr"}
};

std::string replace_binary_functions(std::string& line) {
    for (auto pair = binary_functions.begin(); pair != binary_functions.end(); pair++) {
        std::string token = (*pair).first;
        std::string replacement = (*pair).second;

        int pos = 0;
        while ((pos = line.find(token, pos)) != std::string::npos) {
            int param0_end = line.substr(0, pos).find_last_not_of(WHITESPACE);
            // If it's not alphanumeric, skip (e.g. don't want to change `value = 3 % 2`
            if (param0_end == std::string::npos || !(
                is_alphanumeric(line[param0_end]) ||
                line[param0_end] == ')' ||
                line[param0_end] == ']' // TODO col[2] % 3
            )) {
                pos+=token.size();
                continue;
            }

            // logger << line << ENDL;

            int param0_start = param0_end;
            param0_end++;

            // If it's a parenthesis, just follow it
            if (line[param0_start] == ')') {
                int stack = 1;
                for (param0_start = param0_start-1; stack > 0 && param0_start >= 0; param0_start--) {
                    if (line[param0_start] == ')')
                        stack++;
                    else if (line[param0_start] == '(')
                        stack--;
                }
                if (stack > 0) {
                    // Multiline. I ain't doing that.
                    logger << "TODO Multiline binary operator param0" << ENDL;
                    pos += token.size();
                    continue;
                }
                // undo the last -- of the loop, we want to target the closing (
                param0_start++;
                // We need to check whether the preceeding token is something that could be a function name (case `myFun0(12) >> 3`)
                if (is_alphanumeric(line[param0_start - 1])) {
                    param0_start = line.substr(0, param0_start).find_last_not_of(ALPHANUMERIC) + 1;
                }
            }
            else {
                param0_start = line.substr(0, param0_start).find_last_not_of(ALPHANUMERIC)+1;
            }

            int paramf_start = line.find_first_not_of(WHITESPACE, pos + token.size());
            // That must be alphanumeric, otherwise skip
            if (paramf_start == std::string::npos || !(
                is_alphanumeric(line[paramf_start]) ||
                line[paramf_start] == '('
            )) {
                // logger << "paramf not alphanumeric" << ENDL;
                pos += token.size();
                continue;
            }
            int paramf_end = paramf_start;

            // If it's a parenthesis, just follow it
            if (line[paramf_end] == '(') {
                int stack = 1;
                for (paramf_end = paramf_end + 1; stack > 0 && paramf_end < line.size(); paramf_end++) {
                    if (line[paramf_end] == '(')
                        stack++;
                    else if (line[paramf_end] == ')')
                        stack--;
                }
                if (stack > 0) {
                    // Multiline. I ain't doing that.
                    logger << "TODO Multiline binary operator paramf" << ENDL;
                    pos += token.size();
                    continue;
                }
            }
            else {
                paramf_end = line.find_first_not_of(ALPHANUMERIC, paramf_end);
                // TODO function call
                if (paramf_end == std::string::npos) {
                    paramf_end = line.size();
                }
            }

            line = line.substr(0, param0_start) + " " + replacement + "(" +
                line.substr(param0_start, param0_end - param0_start) + "," +
                line.substr(paramf_start, paramf_end - paramf_start) + ")" + line.substr(paramf_end);
            // logger << "=> " << line << ENDL;
        }
    }
    return line;
}

std::map<unsigned char, unsigned short> char_to_pattern = {
    {0x80, 0x0000}, //█
    {0x81, 0xA5A5}, //▒
    {0x82, 0xE1FA}, //🐱
    {0x83, 0x0003}, //⬇️
    {0x84, 0xD7D7}, //░
    {0x85, 0x81DB}, //✽
    {0x86, 0x99FF}, //
    {0x87, 0xBF51}, //
    {0x88, 0x5BFB}, //
    {0x89, 0x99F9}, //
    {0x8A, 0x11FB}, //
    {0x8B, 0x0000}, //
    {0x8C, 0x0E0A}, //
    {0x8D, 0x9B3F}, // ♪
    {0x8E, 0x0004}, //
    {0x8F, 0xB1BF}, //
    {0x90, 0x5FFF}, //
    {0x91, 0x0001}, //
    {0x92, 0xB15F}, //
    {0x93, 0x1B1F}, //
    {0x94, 0x0002}, //
    {0x95, 0xF5BF}, //
    {0x96, 0x7ADF}, //
    {0x97, 0x0005}, //
    {0x98, 0x0F0F}, //
    {0x99, 0x5555}, // ▥
};

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

        // Remove comments ( -- blah). They are not special, but sometimes they can break assumptions
        // made on other mods (e.g. x+=3--5 should become x=x+(3)--5, but instead becomes x+=(3--5) which is invalid lua
        int comment = line.find("--");
        if (comment != std::string::npos && line.find("--[[") == std::string::npos && line.find("]]") == std::string::npos) {
            // TODO improve
            std::string prefix = line.substr(0, comment);
            int quote_count = std::count(prefix.begin(), prefix.end(), '"');
            if(quote_count % 2 == 0) {
                line = prefix;
            }
        }

        line = replace_escape_chars(line);

        if (line.find("btn(") != std::string::npos || line.find("btnp(") != std::string::npos) {
            bool replaced = true;
            while (replaced) {
                replaced = false;
                for (const auto& myPair : button_to_key) {
                    unsigned char key = myPair.first;
                    if ((pos = line.find(key)) != std::string::npos) {
                        line = line.replace(pos, 1, std::to_string((int)button_to_key[key]));
                        replaced = true;
                    }
                }
            }
        }

        if ((pos = line.find("fillp(")) != std::string::npos) {
            pos += 6;
            if (char_to_pattern.count(line[pos]) > 0) {
                line = line.substr(0, pos) + std::to_string(char_to_pattern[line[pos]]) + line.substr(pos+1);
            }
        }

        // != => ~=
        pos = 0;
        while ((pos = line.find("!=", pos)) != std::string::npos) {
            line.replace(pos, 2, "~=");
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

        line = replace_assignment_operators(line);

        // if (condition) something => if (condition) then something end
        if (((pos = line.find("if(")) != std::string::npos ||
            (pos = line.find("if (")) != std::string::npos) &&
            (pos == 0 || line[pos-1] == ' ' || line[pos-1] == '\t') && // case value = someFn(iif(parameter, 1, 2))
            line.find("then", pos) == std::string::npos) {
            // Look for closing )
            int stack = 1;
            pos += 4;
            while (pos < line.length() && stack > 0) {
                if (line[pos] == '(') {
                    stack++;
                }
                if (line[pos] == ')') {
                    stack--;
                }
                pos++;
            }
            if (pos < line.length() && line.find_first_not_of(" ", pos) != std::string::npos &&
                // We need to exclude if line ends with or or and
                line.find(" or",  pos) != line.size()-3 && line.find(" and", pos) != line.size()-4) {
                line = line.replace(pos, 0, " then ") + " end";
            }
        }

        // if something > 3then => if something > 3 then
        // WTF?!?
        if (((pos = line.find("then")) != std::string::npos) && pos > 0 &&
            line[pos-1] >= '0' && line[pos-1] <= '9') {
            line = line.replace(pos, 0, " ");
        }

        // if for i=1,5do => for i=1,5 do
        if (((pos = line.find("for")) != std::string::npos) &&
            ((pos = line.find("do", pos)) != std::string::npos) &&
            line[pos - 1] >= '0' && line[pos - 1] <= '9') {
            line = line.replace(pos, 0, " ");
        }

        line = replace_binary_functions(line);

        out << line << ENDL;
        line_num++;
    }

    return out.str();
}