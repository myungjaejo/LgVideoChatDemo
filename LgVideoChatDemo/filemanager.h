#pragma once

#include "definition.h"
bool StoreData(const TRegisteration* data, size_t size);
bool LoadData(TRegisteration* data, size_t size);

void test_filemanager();
