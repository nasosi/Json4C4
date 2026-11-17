#include <Json4C4/C4Json.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size );

int main( int argumentCount, char** arguments )
{
    for ( int a = 1; a < argumentCount; a++ )
    {
        Terathon::Array<char> text;

        if ( C4::Json::ReadTextFile( arguments[ a ], text ) != C4::Json::Status::kOk )
        {
            fprintf( stderr, "Cannot read %s\n", arguments[ a ] );
            return 1;
        }

        printf( "%s\n", arguments[ a ] );
        fflush( stdout );

        LLVMFuzzerTestOneInput( reinterpret_cast<const uint8_t*>( text.begin() ), size_t( text.GetArrayElementCount() ) );
    }

    return 0;
}
