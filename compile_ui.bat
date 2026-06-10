call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl.exe /c /EHsc /std:c++17 /I include src\UIManager.cpp
