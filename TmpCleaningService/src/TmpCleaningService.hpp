#pragma once

#include <Windows.h>
#include <string>

namespace TmpCleaningService {
    const wchar_t* GetServiceName();
    const wchar_t* GetServiceDisplayName();
    SERVICE_TABLE_ENTRY GetServiceTableEntry();
}

