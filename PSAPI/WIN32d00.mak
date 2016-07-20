.autodepend

all:   cfgexe  Test.exe   cfgcln
 @echo SYSTEM=WIN32 MODEL=d DIAGS=0 DEBUG=0

.rc.res:
 C:\BC5\bin\brcc32.exe -r -i;C:\BC5\include $<
.cpp.obj:
 C:\BC5\bin\bcc32.exe {$< }
.c.obj:
 C:\BC5\bin\bcc32.exe {$< }



cfgexe:
 @copy &&|
-I;C:\BC5\include
-c  -W -D_RTLDLL -3 -i55 -d -k- -O1gmpv -WM- -a1
-w   
| bcc32.cfg >NUL



Test.exe: Test.obj   psapi.lib 
  C:\BC5\bin\tlink32 @&&|
/V3.10 -Tpe -aa -c  -L C:\BC5\lib\c0w32+
Test.obj
Test.exe
-x
psapi.lib C:\BC5\lib\import32 C:\BC5\lib\cw32i
|,&&|
EXETYPE WINDOWS
CODE PRELOAD MOVEABLE DISCARDABLE
DATA PRELOAD MOVEABLE MULTIPLE
|

cfgcln:
 @del bcc32.cfg

