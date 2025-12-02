#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int nArgc, char** lppszArgv )
{
    int nMode;

    if( nArgc < 2 )
    {
        return -1;
    }

    if( strcmp( lppszArgv[1], "magic_param" ) != 0 )
    {
        return -1;
    }

    if( nArgc > 2 )
    {
        switch( atoi( lppszArgv[2] ) )
        {
        case 0:
            // pretend we changed it
            return 0;
        case 1:
            // pretend we changed it
            return 1;
        default:
            return -1;
        }
    }

    // pretend it's disabled
    return 0;
}
