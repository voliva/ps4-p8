#pragma once

#include <string>

// Helper to write/read a save state
void write_state(unsigned char* dest, int* offset, const void* src, int length);
void read_state(void* dest, const unsigned char* src, int* offset, int length);

void initialize_save_states();

bool save_state_exists(std::string cartridgeName);

void save_state(std::string cartridgeName);
void load_state(std::string cartridgeName);
