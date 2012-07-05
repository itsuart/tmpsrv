#define UNICODE
#define SUCCESS 0

#include <windows.h>

static USHORT* TempDir = L"c:\\tmp";

void Main(){
	ExitProcess(SUCCESS);
}