
#include <errno.h>
#include "io/FileIOClient.h"
#include "io/FileUtils.h"
#include "util/HexDumpBuffer.h"

using namespace oasys;

int
main(int argc, const char** argv)
{
    Log::init();
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        exit(1);
    }

    const char* file = argv[1];

    if (!FileUtils::readable(file)) {
        fprintf(stderr, "can't read file %s\n", file);
        exit(1);
    }

    FileIOClient f;
    if (f.open(file, O_RDONLY | O_EXCL) < 0) {
        fprintf(stderr, "error opening file: %s\n", strerror(errno));
        exit(1);
    }

    int size = FileUtils::size(file);
    HexDumpBuffer h(size);
    h.append(&f, size);
    f.close();

    h.hexify();
    printf("%s", h.c_str());
}

     
    
