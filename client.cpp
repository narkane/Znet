#include "ZFIP.h"

int main(int argc, char* argv[])
{
    client c1(8888, argv[1]);
    c1.conn();
    for(int i=0; i < 3; i++)
    {
        c1.readWrite();
    }
    c1.closeConn();
}
