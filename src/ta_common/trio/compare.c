#include "trio.h"

#define compare(format, value) printf("FORMAT: %s\n", format); printf("TRIO: "); trio_printf(format,value); printf("\nLIBC: "); \
printf(format,value); printf("\n\n"); 

int main()
{
   compare("\"%e\"",2.342E+02);
   compare("\"%10.4e\"",-2.342E-02);
   compare("\"%11.4e\"",-2.342E-02);
   compare("\"%12.4e\"",-2.342E-02);
   compare("\"%13.4e\"",-2.342E-02);
   compare("\"%14.4e\"",-2.342E-02);
   compare("\"%15.4e\"",-2.342E-02);
   compare("\"%16.4e\"",-2.342E-02);
   compare("\"%16.4e\"",-2.342E-22);
   compare("\"%G\"",-2.342E-02);
   compare("\"%G\"",3.1415e-6);
   compare("%016e", 3141.5);
   compare("%16e", 3141.5);
   compare("%-16e", 3141.5);
   compare("%010.3e", 3141.5);

   compare("*%5f*", 3.3);
   compare("*%5f*", 3.0);
   compare("*%5f*", .999999E-4);
   compare("*%5f*", .99E-3);
   compare("*%5f*", 3333.0);

   compare("*%5g*", 3.3);
   compare("*%5g*", 3.0);
   compare("*%5g*", .999999E-4);
   compare("*%5g*", .99E-3);
   compare("*%5g*", 3333.0);
   compare("*%5g*", 0.01);

   compare("*%5.g*", 3.3);
   compare("*%5.g*", 3.0);
   compare("*%5.g*", .999999E-4);
   compare("*%5.g*", 1.0E-4);
   compare("*%5.g*", .99E-3);
   compare("*%5.g*", 3333.0);
   compare("*%5.g*", 0.01);

   compare("*%5.2g*", 3.3);
   compare("*%5.2g*", 3.0);
   compare("*%5.2g*", .999999E-4);
   compare("*%5.2g*", .99E-3);
   compare("*%5.2g*", 3333.0);
   compare("*%5.2g*", 0.01);

   return 0;
}
