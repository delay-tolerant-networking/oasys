
#include <stdio.h>
#include <string.h>
#include <util/MD5.h>

using namespace oasys;

// works like md5sum
int
main(int argc, const char** argv)
{
    char buf[256];
    MD5 md5;
    
    while (1) {
        char* line = fgets(buf, sizeof(buf), stdin);
        if (!line)
            break;
        md5.update((u_char*)line, strlen(line));
    }

    md5.finalize();
    std::string digest = md5.digest_ascii();
    
    printf("%s\n", digest.c_str());
}
