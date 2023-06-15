#pragma once

#include "definition.h"
bool StoreData(const TRegistration* data, size_t size);
bool LoadData(TRegistration* data, size_t size);

void test_filemanager();
