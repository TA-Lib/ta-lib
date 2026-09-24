set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER i686-linux-gnu-gcc)

# gcc's i686 default is x87 math, whose extra precision changes values and breaks
# stream == batch. Nothing runs the i386 library, so losing this goes unnoticed.
set(CMAKE_C_FLAGS_INIT "-msse2 -mfpmath=sse")
