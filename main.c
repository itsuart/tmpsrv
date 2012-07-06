#define UNICODE
#define SUCCESS 0

#include <windows.h>

static USHORT* TempDir = L"c:\\tmp";

static void WriteString(USHORT* string){
	DWORD dummy;
	HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleW(stdout, string, lstrlenW(string), &dummy, NULL);
}

static void DisplayLastError(){
			DWORD error = GetLastError();
		// Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;


    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    WriteString(lpMsgBuf);

    LocalFree(lpMsgBuf);
}

/*
	Returns TRUE if file doesn't exists anymore
	Returns FALSE if file still exists.
*/
static BOOL ReallyRemoveFileW(USHORT* file){
	SetLastError(0);
	SetFileAttributesW(file, FILE_ATTRIBUTE_HIDDEN);
	DisplayLastError();

	SetLastError(0);
	DeleteFileW(file);
	DisplayLastError();
	
	return FALSE;
}

static BOOL CreateTmpDir(){
	BOOL result = CreateDirectoryW(TempDir, NULL);
	if (!result){
		//ERROR_ALREADY_EXISTS will be returned if *anything* with such name already exists
		//in my case directory or file
		//I can't believe Windows won OS competition.
		//It's api is plainly was designed by retards. If was designed at all.
		//BEWARE: in order to remove file, ensure, that it doesn't have RO attribute
		//or DeleteFileW will fail.
		return ERROR_ALREADY_EXISTS == GetLastError();
	} else {
		return TRUE;
	}

	return FALSE;
}

static BOOL RemoveTmpDir(){
	// unfortunately, RemoveDirectoryW can't remove non-empty directories :(
	return DeleteFileW(TempDir);
	//return FALSE;
}


void Main(){

	WriteString(L"Removing file...");
	if (ReallyRemoveFileW(TempDir)){
		WriteString(L"Success\n");
	} else {
		WriteString(L"Failed");
	}
ExitProcess(SUCCESS);
return;
	BOOL succeeded = CreateTmpDir();
	if (succeeded){
		WriteString(L"Creation succeeded\n");
		if (RemoveTmpDir()){
			WriteString(L"Removing succeeded\n");
		}
	}
	
}