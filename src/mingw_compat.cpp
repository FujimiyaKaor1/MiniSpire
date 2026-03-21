#include <stdio.h>

extern "C" {
    FILE* __imp___iob_func[3] = {
        stdin,
        stdout,
        stderr
    };
}
