#pragma once

#include "definition.h"
#include <vector>

bool StoreData(const TRegistration* data, size_t size);
bool StoreData(std::vector<TRegistration*> data);
bool LoadData(TRegistration* data, int idx);
int getLengthJSON(void);
void printFileObj(const TRegistration* data);

// void test_filemanager();