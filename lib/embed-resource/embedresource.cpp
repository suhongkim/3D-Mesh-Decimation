#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <iostream>

using namespace std;
using namespace boost::filesystem;

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s {sym} {rsrc}\n\n"
                        "  Creates {sym}.c from the contents of {rsrc}\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    path dst{argv[1]};
    path src{argv[2]};

    string sym = src.filename().string();
    replace(sym.begin(), sym.end(), '.', '_');

    create_directories(dst.parent_path());

    boost::filesystem::ofstream ofs{dst};

    boost::filesystem::ifstream ifs{src};

    ofs << "#include <stdlib.h>" << endl;
    ofs << "const char _resource_" << sym << "[] = {" << endl;

    size_t lineCount = 0;
    while (!ifs.eof())
    {
        char c;
        ifs.get(c);
        ofs << "0x" << hex << (int)c << ", ";
        if (++lineCount == 10) {
            ofs << endl;
            lineCount = 0;
        }
    }


    ofs << "};" << endl;
    ofs << "const size_t _resource_" << sym << "_len = sizeof(_resource_" << sym << ");";

    return EXIT_SUCCESS;
}