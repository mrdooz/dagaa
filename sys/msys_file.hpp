#pragma once
#if WITH_FILE_UTILS
#include <vector>
bool LoadFile(const char* filename, std::vector<char> *buf);

#endif