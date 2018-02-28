
#include <iostream>
#include <boost/lexical_cast.hpp>

#include "../bin/version.h"

#include "processor.h"

int main(int argc, char** argv)
{
    size_t N = argc > 1 ? boost::lexical_cast<size_t>(argv[1]) : 0;

    Reader r(N);
    ConsolePrint cp(r);
    FilePrint fp(r);

    r.read(std::cin);

    return 0;
}
