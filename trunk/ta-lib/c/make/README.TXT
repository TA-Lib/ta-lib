These directories contains the Makefiles
for building the TA-LIB.

The first level of directory allows to identify
the kind of application environment in which the
library is going to be used:

Example:
  CSR - For application single thread
  CMR - For application multi  thread
  CSD - For application single thread wiht debug options
  CMD - For application multi  thread with debug options
  CSD - For application single thread wiht profiling options
  CMD - For application multi  thread with profiling options

Choose the most appropriate directory depending of
your platform/compiler and run "make" or "nmake"
from there.

See the API Documentation for more information
on how to build and link with TA-LIB.

The directory gen_make contains the perl scripts
automating the generation of the makefiles.
