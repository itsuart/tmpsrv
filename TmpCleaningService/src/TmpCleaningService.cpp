#include "TmpCleaningService.hpp"
#include <vector>
#include <string>
#include <wtsapi32.h>

namespace TmpCleaningService {
    static void DebugPrintError(DWORD error) {
        wchar_t* reason = NULL;

        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPWSTR)&reason, 1, NULL);
        OutputDebugStringW(reason);
        OutputDebugStringW(L"\n");
        HeapFree(GetProcessHeap(), 0, reason);
    }

    static DWORD CountLoggedInUsers() {
        DWORD level = 1;
        WTS_SESSION_INFO_1W* sessionInfos = nullptr;
        DWORD nSessionInfos = 0;
        if (!WTSEnumerateSessionsExW(WTS_CURRENT_SERVER_HANDLE, &level, 0, &sessionInfos, &nSessionInfos)) {
            auto lastError = GetLastError();
            OutputDebugStringA("WTSEnumerateSessionsExW failed:");
            DebugPrintError(lastError);

            return 0; //TODO: I think this will require call Lsa SomethingSomething
        }

        DWORD nLoggedInUsers = 0;
        for (int i = 0; i < nSessionInfos; ++i) {
            if ((sessionInfos + i)->pUserName != nullptr) {
                nLoggedInUsers += 1;
            }
        }

        WTSFreeMemoryExW(WTSTypeSessionInfoLevel1, sessionInfos, nSessionInfos);
        sessionInfos = nullptr;
        nSessionInfos = 0;

        return nLoggedInUsers;
    }

    static bool is_two_dots(const wchar_t* string) {
        return string[0] == L'.' && string[1] == L'.' && string[2] == 0;
    }

    static void DeleteFolderContents(const std::wstring& currentFolder) {
        const std::wstring searchFolder = currentFolder + L"\\*";
        WIN32_FIND_DATAW findData;
        HANDLE searchHandle = FindFirstFileExW(
            searchFolder.c_str(),
            FindExInfoBasic,
            &findData,
            FindExSearchNameMatch,
            nullptr, FIND_FIRST_EX_LARGE_FETCH
        );

        if (INVALID_HANDLE_VALUE == searchHandle) {
            // there is nothing we can do but skip this folder altogether
            return;
        }

        std::vector<std::wstring> subFolders;

        bool keepSearching = true;
        bool hasError = false;
        while (keepSearching) {
            if (FindNextFileW(searchHandle, &findData)) {
                //ignore directory junctions: don't want to accidentedly delete referenced content
                const DWORD attributes = findData.dwFileAttributes;
                const bool isDirectory = ((attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
                const bool isReparsePoint = ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT);

                if (is_two_dots(findData.cFileName) || isReparsePoint) {
                    continue;
                }

                std::wstring currentItemFullPath = currentFolder + L"\\" + findData.cFileName;
                if (isDirectory) {
                    subFolders.push_back(currentItemFullPath);
                }
                else {
                    DeleteFileW(currentItemFullPath.c_str()); // we do ignore failures
                }

            }
            else {
                //what went wrong?
                if (ERROR_NO_MORE_FILES == GetLastError()) {
                    //that's ok
                    keepSearching = false;
                }
                else {
                    hasError = true;
                    keepSearching = false;
                }
            }
        }

        FindClose(searchHandle);

        for (const auto& subFolder : subFolders) {
            DeleteFolderContents(subFolder);
            RemoveDirectoryW(subFolder.c_str());
        }
    }

    static void ClearTheFolder() {
        const auto theFolder = L"c:\\tmp";
        if (!CreateDirectoryW(theFolder, nullptr)) { // if folder exists
            DeleteFolderContents(theFolder);
        }
    }

    class CleaningService final {
        private:
            SERVICE_STATUS_HANDLE m_serviceUpdateHandle = nullptr;
            HANDLE m_stopEvent = nullptr;
            SERVICE_STATUS m_currentServiceStatus;


            bool IsProperlyInitialized() const noexcept {
                return (m_stopEvent != nullptr) && (m_serviceUpdateHandle != nullptr);
            }

            void UpdateServiceStatus(DWORD statusCode) {
                m_currentServiceStatus.dwCurrentState = statusCode;
                SetServiceStatus(m_serviceUpdateHandle, &m_currentServiceStatus);
            }

        public:
            explicit CleaningService() noexcept {
                m_currentServiceStatus.dwCheckPoint = 0;
                m_currentServiceStatus.dwControlsAccepted =
                    SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_POWEREVENT;
                m_currentServiceStatus.dwCurrentState = SERVICE_STOPPED;
                m_currentServiceStatus.dwServiceSpecificExitCode = 0;
                m_currentServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
                m_currentServiceStatus.dwWaitHint = 10* 1000; // 10 seconds
                m_currentServiceStatus.dwWin32ExitCode = 0;

                m_stopEvent = CreateEvent(nullptr, false, false, nullptr);
                if (m_stopEvent) {
                    m_serviceUpdateHandle =
                        RegisterServiceCtrlHandlerExW(GetServiceName(), &CleaningServiceMessageHandler, this);

                    if (m_serviceUpdateHandle) {
                        UpdateServiceStatus(SERVICE_START_PENDING);

                        // do some initialization here

                        UpdateServiceStatus(SERVICE_RUNNING);
                    }
                }

            }

            ~CleaningService() noexcept {
                CloseHandle(m_stopEvent);
                m_stopEvent = nullptr;
                m_serviceUpdateHandle = nullptr;
            }

            void Run() noexcept {
                if (!IsProperlyInitialized()) return;
                const DWORD retCode = WaitForSingleObject(m_stopEvent, INFINITE);



                switch (retCode) {
                    case WAIT_OBJECT_0: {
                        // stop event was signaled
                    } break;

                    case WAIT_ABANDONED: {
                        //only happens for mutexes
                    } break;

                    case WAIT_TIMEOUT: {
                        // impossible
                    } break;

                    case WAIT_FAILED: {
                        const auto error = GetLastError();
                        OutputDebugStringA("WAIT_FAILED\n");
                        DebugPrintError(error);
                    } break;
                }

                UpdateServiceStatus(SERVICE_STOPPED);
            }

            DWORD HandleControlMessage(DWORD dwControl, DWORD dwEventType, void* eventData) noexcept {
                switch (dwControl) {
                    case SERVICE_CONTROL_INTERROGATE: { //mandatory
                        return NO_ERROR; // SCM knowns our state
                    } break;

                    case SERVICE_CONTROL_POWEREVENT: {
                        if (dwEventType == PBT_APMSUSPEND) {
                            // why?
                            if (CountLoggedInUsers() > 0) {
                                //there are still users connected. Looks like this is sleep call
                                // do nothing
                            }
                            else {
                                // if there are no users left this is probably shutdown
                                ClearTheFolder();
                            }
                        }
                        return NO_ERROR;
                    } break;

                    case SERVICE_CONTROL_SHUTDOWN: {
                        UpdateServiceStatus(SERVICE_STOP_PENDING);

                        //System is going to shutdown, clean the tmp folder
                        ClearTheFolder();

                        SetEvent(m_stopEvent);
                        return NO_ERROR;
                    } break;

                    case SERVICE_CONTROL_STOP: {
                        UpdateServiceStatus(SERVICE_STOP_PENDING);
                        SetEvent(m_stopEvent);
                        return NO_ERROR;
                    } break;

                    default: {
                        OutputDebugStringA("UNKNOWN SERVICE CONTROL MESSAGE\n");
                        return ERROR_CALL_NOT_IMPLEMENTED;
                    } break;
                }
            }

            static DWORD __stdcall CleaningServiceMessageHandler(
                DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {

                CleaningService* pService = reinterpret_cast<CleaningService*>(lpContext);
                return pService->HandleControlMessage(dwControl, dwEventType, lpEventData);
            }

            CleaningService(const CleaningService&) = delete;
            CleaningService(CleaningService&&) = delete;

            CleaningService& operator=(const CleaningService&) = delete;
            CleaningService& operator=(CleaningService&&) = delete;
    };



    static void __stdcall CleaningServiceMain(DWORD argc, wchar_t** argv) {
        CleaningService service;
        service.Run();
    }

    const wchar_t * GetServiceName(){
        return L"TmpCleaningService";
    }

    const wchar_t * GetServiceDisplayName(){
        return nullptr;
    }

    SERVICE_TABLE_ENTRY GetServiceTableEntry(){
        return { (LPWSTR)GetServiceName(), CleaningServiceMain };
    }

}
