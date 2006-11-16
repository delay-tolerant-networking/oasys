
#include "config.h"

#if OASYS_ZLIB_ENABLED

#include <sys/errno.h>
#include <zlib.h>

#include "../debug/Log.h"
#include "../io/FileUtils.h"
#include "../io/MmapFile.h"
#include "../util/Getopt.h"
#include "../util/MD5.h"


using namespace oasys;

int
main(int argc, char* const argv[])
{
    const char* LOG = "/zlibsize";
    
    std::string filename = "";
    u_int       len    = 0;
    u_int       offset = 0;

    oasys::Getopt opts;
    opts.addopt(new UIntOpt('L', "length", &len));
    opts.addopt(new UIntOpt('o', "offset", &offset));
    
    int remainder = opts.getopt(argv[0], argc, argv);
    if (remainder != argc - 1) {
        fprintf(stderr, "invalid argument '%s'\n", argv[remainder]);
        opts.usage(argv[0], "filename");
        exit(1);
    }

    filename = argv[remainder];
        
    Log::init();

    if (filename == "") {
        log_err(LOG, "filename must be set");
        exit(1);
    }

    if (len == 0) {
        len = FileUtils::size(filename.c_str());
    }

    MmapFile mm("/zlibsize/mmap");
    const u_char* src = (const u_char*)mm.map(filename.c_str(), PROT_READ, 0,
                                              len, offset);
    if (src == NULL) {
        log_err(LOG, "error mmap'ing file: %s", strerror(errno));
        exit(1);
    }

    unsigned long zlen = compressBound(len);
    void* dst = malloc(zlen);
    log_debug(LOG, "calling compress on %lu byte buffer: src len %u",
              zlen, len);
    
    int cc = compress((Bytef*)dst, &zlen, src, len);
    if (cc != Z_OK) {
        log_err(LOG, "error in compress: %s", zError(cc));
        exit(1);
    }
    free(dst);

    printf("%s\t%u\t%lu\n", filename.c_str(), len, zlen);
    fflush(stdout);
}

#else // OASYS_ZLIB_ENABLED

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char* const argv[])
{
    (void)argc;
    fprintf(stderr, "%s: zlib required\n", argv[0]);
    exit(1);
}

#endif // OASYS_ZLIB_ENABLED
