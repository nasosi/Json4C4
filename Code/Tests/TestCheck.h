#ifndef TestCheck_h
#define TestCheck_h

#include <Json4C4/C4Json.h>

#include <cstdio>
#include <cstring>

namespace Json = C4::Json;

constexpr int testSkippedReturnCode = 77;

inline int testFailureCount = 0;

inline void Check( bool condition, const char* description )
{
    if ( !condition )
    {
        fprintf( stderr, "FAILED: %s\n", description );
        testFailureCount++;
    }
}

inline void CheckStatus( Json::Status status, Json::Status expected, const char* description )
{
    if ( status != expected )
    {
        fprintf( stderr,
                 "FAILED: %s: status %u (%s), expected %u (%s)\n",
                 description,
                 unsigned( status ),
                 static_cast<const char*>( Json::StatusToString( status ) ),
                 unsigned( expected ),
                 static_cast<const char*>( Json::StatusToString( expected ) ) );
        testFailureCount++;
    }
}

inline int TestResult()
{
    if ( testFailureCount != 0 )
    {
        fprintf( stderr, "%d checks failed\n", testFailureCount );
        return 1;
    }

    return 0;
}

inline Terathon::Array<char> MakeTextBuffer( const char* text, size_t length )
{
    Terathon::Array<char> buffer;
    buffer.SetArrayElementCount( Terathon::int32( length + 1 ) );

    for ( size_t a = 0; a < length; a++ )
    {
        buffer[ Terathon::int32( a ) ] = text[ a ];
    }

    buffer[ Terathon::int32( length ) ] = 0;

    return buffer;
}

inline Json::ParseResult ParseText( Json::StructuredData& sd, const char* text, size_t length, const Json::ParseOptions& options = Json::ParseOptions() )
{
    return sd.Parse( MakeTextBuffer( text, length ), options );
}

inline Json::ParseResult ParseText( Json::StructuredData& sd, const char* text, const Json::ParseOptions& options = Json::ParseOptions() )
{
    return ParseText( sd, text, strlen( text ), options );
}

inline bool ReadTextFile( const char* fileName, Terathon::String<>& text )
{
    FILE* file = fopen( fileName, "rb" );

    if ( !file )
    {
        return false;
    }

    text = "";

    char   buffer[ 4096 ];
    size_t count;

    while ( ( count = fread( buffer, 1, sizeof( buffer ) - 1, file ) ) != 0 )
    {
        buffer[ count ] = 0;
        text += buffer;
    }

    fclose( file );

    return true;
}

inline bool WriteTextFile( const char* fileName, const char* text )
{
    FILE* file = fopen( fileName, "wb" );

    if ( !file )
    {
        return false;
    }

    size_t length = strlen( text );
    bool   ok     = fwrite( text, 1, length, file ) == length;

    fclose( file );

    return ok;
}

#endif
