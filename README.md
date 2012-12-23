## What Is It?

Simple Windows Service, that will clean c:\tmp folder upon system shutdown and startup. Like /tmp on Mac OS X and Linux systems.

## How To Use?

1. Put executable ('tmpsrv.exe') somewhere (for example in 'c:\tools\').
2. Start Windows console (cmd.exe) as Administrator and navigate to directory with executable.
3. To install service type tmpsrv.exe install.
4. To uninstall service type tmpsrv.exe uninstall.

## How To Build?

This is a bit complicated:

1. Go to http://www.mingw.org/, download and install MinGW.
2. Navigate to the directory where you put sources of the service.
3. If you installed MinGW not into C:\MinGW, you will need update build.bat accordingly: replace -L"c:\MinGW\lib" with path to lib within your custom MinGW installation.
4. Run build.cmd.

Also, you might try VisualC++, but you most likely will need update code a little, since it doesn't support (at the time of the writing) C99.

## Are there prebuild binaries?
Yes (32bit but it works just fine on my 64bit Windows8): https://dl.dropbox.com/u/2858326/software/tmpsrv/tmpsrv.exe

## License?
MIT. File LICENSE contains a copy.
