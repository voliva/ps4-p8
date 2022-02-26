#include "save_states.h"
#include <string>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <unistd.h>
#endif
#include "log.h"
#include "audio.h"
#include "lua_state.h"
#include "machine_state.h"

#ifdef __PS4__
#define SAVE_STATES_FOLDER "/data/p8-savestates"
#else
#define SAVE_STATES_FOLDER "../p8-savestates"
#endif


#define DEBUGLOG SaveStates_DEBUGLOG
Log DEBUGLOG = logger.log("Save States");

void write_state(unsigned char* dest, int* offset, const void* src, int length) {
	memcpy(&dest[*offset], src, length);
	*offset += length;
}
void read_state(void* dest, const unsigned char* src, int* offset, int length) {
	memcpy(dest, &src[*offset], length);
	*offset += length;
}

typedef struct {
	std::string name;
	bool exists;
	unsigned char* data;
} SaveStateCache;

SaveStateCache saveStateCache{};

void initialize_save_states()
{
	saveStateCache.exists = false;
	saveStateCache.data = NULL;

	struct stat st = { 0 };
	if (stat(SAVE_STATES_FOLDER, &st) == -1) {
		mkdir(SAVE_STATES_FOLDER, 0700);
	}
}

bool save_state_exists(std::string cartridgeName)
{
	if (saveStateCache.name == cartridgeName) {
		return saveStateCache.exists;
	}
	saveStateCache.name = cartridgeName;
	saveStateCache.exists = false;
	if (saveStateCache.data != NULL)
		free(saveStateCache.data);
	saveStateCache.data = NULL;

	std::string path = std::string(SAVE_STATES_FOLDER) + "/" + cartridgeName + ".dat";
	FILE* f = fopen(path.c_str(), "rb");
	if (f) {
		fseek(f, 0, SEEK_END); // seek to end of file
		size_t size = ftell(f); // get current file pointer
		fseek(f, 0, SEEK_SET); // seek back to beginning of file
		saveStateCache.data = (unsigned char *)malloc(size);
		fread(saveStateCache.data, 1, size, f);
		fclose(f);
		saveStateCache.exists = true;
	}
	else {
		DEBUGLOG << "Could not load, error: " << errno << ENDL;
	}
	return saveStateCache.exists;
}

void save_state(std::string cartridgeName)
{
	saveStateCache.name = cartridgeName;
	if (saveStateCache.data != NULL)
		free(saveStateCache.data);
	size_t size = 1 + audioManager->getSize() + machineState->getSize() + luaState->getSize();
	saveStateCache.data = (unsigned char*)malloc(size);
	if (saveStateCache.data == 0) {
		DEBUGLOG << "Can't allocate memory to save" << ENDL;
		saveStateCache.exists = false;
		return;
	}
	memset(saveStateCache.data, 0, size);

	saveStateCache.data[0] = 0; // Version
	int offset = 1;

	audioManager->serialize(&(saveStateCache.data[offset]));
	offset += audioManager->getSize();

	machineState->serialize(&(saveStateCache.data[offset]));
	offset += machineState->getSize();

	luaState->serialize(&(saveStateCache.data[offset]));
	offset += luaState->getSize();

	saveStateCache.exists = true;

	std::string path = std::string(SAVE_STATES_FOLDER) + "/" + cartridgeName + ".dat";
	FILE* f = fopen(path.c_str(), "wb");
	if (f) {
		fwrite(saveStateCache.data, 1, size, f);
		fclose(f);
	}
	else {
		DEBUGLOG << "Could not save, error: " << errno << ENDL;
	}
}

void load_state(std::string cartridgeName)
{
	if (!save_state_exists(cartridgeName)) {
		return;
	}
	char version = saveStateCache.data[0]; // Spot for later
	int offset = 1;

	audioManager->deserialize(&(saveStateCache.data[offset]));
	offset += audioManager->getSize();

	machineState->deserialize(&(saveStateCache.data[offset]));
	offset += machineState->getSize();

	luaState->deserialize(&(saveStateCache.data[offset]));
	offset += luaState->getSize();
}
