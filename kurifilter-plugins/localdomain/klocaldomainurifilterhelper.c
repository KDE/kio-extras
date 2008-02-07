/*
    kshorturifilterhelper.cpp

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Helper for localdomainurifilter for finding out if a host exist */

#ifndef NULL
#define NULL 0
#endif

#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char* argv[] )
{
    struct hostent* ent;

    if( argc != 2 )
      return 2;

    ent = gethostbyname( argv[ 1 ] );
    if (ent)
    {
        int i;
        int found = 0;
        /* try to find the same fully qualified name first */
        for( i = 0;
             ent->h_aliases[ i ] != NULL;
             ++i )
        {
            if( strncmp( argv[ 1 ], ent->h_aliases[ i ], strlen( argv[ 1 ] )) == 0 )
            {
                found = 1;
                fputs( ent->h_aliases[ i ], stdout );
                break;
            }
        }
        if( !found )
            fputs( ent->h_name, stdout );
    }

    return (ent != NULL || h_errno == NO_ADDRESS) ? 0 : 1;
}
