REM clean
rm main.obj main.exe
REM compile
cc --std=c99 -c main.c -o main.obj
REM link
ld -s --subsystem console --kill-at -e _Main -L"c:\MinGW\lib"  main.obj -o main.exe -lkernel32