## What is it?

Simple Windows Service for Windows 7+, that cleans c:\tmp folder content on system shutdown and reboot. Putting system into sleep, starting or stopping the service will leave the content intact.

## Are there pre-built binaries?
Yes. See Releases tab: https://github.com/itsuart/tmpsrv/releases

## How to use?

1. Extract the executable (TmpCleaningService64.exe for 64bit OSes, TmpCleaningService64.exe for 32bit ones) somewhere (for example in 'c:\tools\').
2. Start it (doubleclick) to install or uninstall the service.

## How to build?

Visual Studio 17 should build this project without any problem.

## License?
MIT. File LICENSE contains a copy.
