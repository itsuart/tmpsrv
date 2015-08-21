@echo off
cls
del main.obj tmpsrv.exe
gcc -O2 -nostartfiles -Wall --std=c99 -c main.c -o main.obj
ld -s --subsystem windows --kill-at --no-seh -e entry_point main.obj -o tmpsrv.exe -L%LIBRARY_PATH% -lkernel32 -lshell32 -ladvapi32 -luser32

