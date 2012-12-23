REM clean
rm main.obj tmpsrv.exe
REM compile
cc -O1 -Wall --std=c99 -c main.c -o main.obj
REM link
ld -s --subsystem console --kill-at --no-seh -e _Main -L"c:\MinGW\lib"  main.obj -o tmpsrv.exe -lkernel32 -lshell32 -ladvapi32