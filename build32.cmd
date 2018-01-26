@echo off
cls
del main.obj tmpsrv32.exe >nul 2>&1
gcc -O2 -nostartfiles -Wall --std=c99 -c main.c -o main.obj
ld -s --subsystem windows --kill-at --no-seh -e _entry_point main.obj -o tmpsrv32.exe -L%LIBRARY_PATH% -lkernel32 -lshell32 -ladvapi32 -luser32
