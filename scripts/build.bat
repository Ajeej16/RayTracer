@echo off

set application_name=ray_tracer
set build_options=
set lib_dir=..\lib
set includes=/I%lib_dir%\glew\include\ /I%lib_dir%\glfw\include\ /I%lib_dir%\cglm\include\ /I%lib_dir%\ocv\include\ /I%lib_dir%\ocv\include\
set libs=/LIBPATH:"%lib_dir%\glew\lib\Release\x64\" /LIBPATH:"%lib_dir%\glfw\lib-static-ucrt\" /LIBPATH:"%lib_dir%\cglm\win\x64\Release\" /LIBPATH:"%lib_dir%\ocv\lib\"
set compiler_flags=/std:c++17 /nologo /Zi /FC /EHsc
set linker_flags=/opt:ref /incremental:no glfw3dll.lib glew32.lib cglm.lib opencv_videoio490.lib opencv_core490.lib opencv_imgproc490.lib opengl32.lib user32.lib gdi32.lib shell32.lib kernel32.lib

set glfw_dll=%lib_dir%\glfw\lib-static-ucrt
set glew_dll=%lib_dir%\glew\bin\Release\x64
set opencv_dll=%lib_dir%\ocv\lib

if not exist ..\build mkdir ..\build
pushd ..\build

copy %glfw_dll%\glfw3.dll .\
copy %glew_dll%\glew32.dll .\
copy %opencv_dll%\opencv_videoio490.dll .\
copy %opencv_dll%\opencv_core490.dll .\
copy %opencv_dll%\opencv_imgproc490.dll .\
copy %opencv_dll%\opencv_imgcodecs490.dll .\

call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"

cl %compiler_flags% %includes% ..\src\main.cpp /link %libs% %linker_flags% /out:%application_name%.exe

popd