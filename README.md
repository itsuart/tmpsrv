## What is it?

Simple Windows Service, that will clean c:\tmp folder upon system shutdown and startup. Like /tmp on Mac OS X and Linux systems.

## How to use?

1. Put executable ('tmpsrv.exe') somewhere (for example in 'c:\tools\').
2. Start it (doubleclick) to install or uninstall the service.

## How to build?

This is a bit complicated:

1. Go to http://mingw-w64.org/ , download and install MinGW.
2. Navigate to the directory where you put sources of the service.
3. Run build32.cmd or build64.cmd to build 32 and 64 bit executables respectfully (you wil need MinGW distribution of proper bitness).

Also, you might try VisualC++, but you will most likely need to update the code a little, since VC++ doesn't support (at the time of the writing) C99.

## Are there pre-built binaries?
Yes. 

32bit: https://dl.dropboxusercontent.com/u/2858326/software/tmpsrv/tmpsrv32.exe

64bit: https://dl.dropboxusercontent.com/u/2858326/software/tmpsrv/tmpsrv64.exe

## License?
MIT. File LICENSE contains a copy.
