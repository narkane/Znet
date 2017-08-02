#include "ZFIP.h"

int main(int argc, char* argv[])
{
    server s1;
    while(true)
    {
        s1.acceptConn();
        s1.awaitMsg();
    }
    //s1.readWrite();
    //s1.closeConn();

    return 0;
}
