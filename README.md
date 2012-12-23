## What Is It?

Simple Windows Service, that will clean c:\tmp folder upon system shutdown and startup.

## How To Use?

1. Put executable ('tmpsrv.exe') somewhere (for example in 'c:\tools\').
2. Start Windows console (cmd.exe) as Administrator and navigate to directory with executable.
3.1 To install service type >tmpsrv.exe install
3.2 To uninstall service type >tmpsrv.exe uninstall

## How To Build?

This is a bit complicated:
1. Go to http://www.mingw.org/, download and install MinGW.
2. Navigate to the directory where you put sources of the service.
3. Run build.cmd.
4. If you installed MinGW not into C:\MinGW, you will need update build.bat accordingly: replace -L"c:\MinGW\lib" with path to lib within your installation.
Also, you might try VisualC++, but you will need update code a little, since it doesn't support (at the time of the writing) C99.

## Are there prebuild binaries?
Yes: https://dl.dropbox.com/u/2858326/software/tmpsrv/tmpsrv.exe

## License?
MIT. File LICENSE contains a copy.
