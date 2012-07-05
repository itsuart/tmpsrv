#define UNICODE
#define SUCCESS 0

#include <windows.h>

static USHORT* TempDir = L"c:\\tmp\\";

static BOOL CreateTmpDir(){
	BOOL result = CreateDirectoryW(TempDir, NULL);
	if (!result){
		//ERROR_ALREADY_EXISTS will be returned if *anything* with such name already exists
		//in my case directory or file
		//I can't believe Windows won OS competition.
		//It's api is plainly was designed by retards. If was designed at all.
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

static void WriteString(USHORT* string){
	DWORD dummy;
	HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleW(stdout, string, lstrlenW(string), &dummy, NULL);
}

void Main(){
	BOOL succeeded = CreateTmpDir();
	if (succeeded){
		WriteString(L"Creation succeeded\n");
		if (RemoveTmpDir()){
			WriteString(L"Removing succeeded\n");
		}
	}
	ExitProcess(SUCCESS);
}