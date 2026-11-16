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

// A hash of results, to compare them with what 2.3.0 gave once 2.x is gone. A status is hashed by its place in a list of
// the tests' own, so that numbering Status differently does not change the hash.
class ResultHash
{
public:
    void Add( const void* bytes, size_t count )
    {
        const unsigned char* byte = static_cast<const unsigned char*>( bytes );

        for ( size_t a = 0; a < count; a++ )
        {
            hash = ( hash ^ byte[ a ] ) * 0x100000001B3ULL;
        }
    }

    void Add( unsigned int value )
    {
        Add( &value, sizeof( value ) );
    }

    void Add( Json::Status status )
    {
        static const Json::Status statuses[] = { Json::Status::kOk,
                                                 Json::Status::kFileOpenError,
                                                 Json::Status::kFileReadError,
                                                 Json::Status::kFileTooLarge,
                                                 Json::Status::kTextBufferContentsInvalid,
                                                 Json::Status::kUnterminatedTextBuffer,
                                                 Json::Status::kExpectedOpeningCurlyBraces,
                                                 Json::Status::kExpectedClosingCurlyBracesOrComma,
                                                 Json::Status::kExpectedOpeningSquareBrackets,
                                                 Json::Status::kExpectedClosingSquareBracketsOrComma,
                                                 Json::Status::kStringIllegalChar,
                                                 Json::Status::kStringIllegalEscape,
                                                 Json::Status::kExpectedColon,
                                                 Json::Status::kExpectedBeginingDoubleQuotes,
                                                 Json::Status::kExpectedEndingDoubleQuotes,
                                                 Json::Status::kFloatInvalid,
                                                 Json::Status::kExpectedTrueOrFalse,
                                                 Json::Status::kExpectedNull,
                                                 Json::Status::kExpectedJsonValue,
                                                 Json::Status::kExpectedEndOfFile,
                                                 Json::Status::kPrematureNullTerminator,
                                                 Json::Status::kNumbersCannotHaveLeadingZeros,
                                                 Json::Status::kInvalidStructuredData,
                                                 Json::Status::kException,
                                                 Json::Status::kInvalidValueType,
                                                 Json::Status::kNameNotPresent,
                                                 Json::Status::kMaximumNestingDepthExceeded,
                                                 Json::Status::kDuplicateName,
                                                 Json::Status::kFileWriteError,
                                                 Json::Status::kMaximumValueCountExceeded,
                                                 Json::Status::kMaximumStringByteCountExceeded,
                                                 Json::Status::kAllocationRefused,
                                                 Json::Status::kStringHoldsZero };

        unsigned int place = 0;

        while ( ( place < sizeof( statuses ) / sizeof( statuses[ 0 ] ) ) && ( statuses[ place ] != status ) )
        {
            place++;
        }

        Add( place );
    }

    void Add( const Json::ParseResult& result )
    {
        Add( result.status );
        Add( unsigned( result.errorLine ) );
        Add( unsigned( result.errorColumn ) );
    }

    // Returns the hash and starts a new one.
    unsigned long long Take()
    {
        unsigned long long value = hash;
        hash                     = start;
        return value;
    }

private:
    static constexpr unsigned long long start = 0xCBF29CE484222325ULL;

    unsigned long long hash = start;
};

// Checks a hash of results against what 2.3.0 gave. With an expected value of 0 the hash is printed, to be filled in.
inline void CheckHash( unsigned long long hash, unsigned long long expected, const char* description )
{
    if ( expected == 0 )
    {
        printf( "%s: 0x%016llXULL\n", description, hash );
    }

    Check( hash == expected, description );
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

inline Json::ParseResult ParseText( Json::V3::StructuredData& data, const char* text, size_t length, const Json::ParseOptions& options = Json::ParseOptions() )
{
    return data.ParseText( text, Terathon::uint64( length ), options );
}

inline Json::ParseResult ParseText( Json::V3::StructuredData& data, const char* text, const Json::ParseOptions& options = Json::ParseOptions() )
{
    return ParseText( data, text, strlen( text ), options );
}

// The text a document writes, with a terminating zero.
inline Terathon::String<> WriteText( const Json::V3::StructuredData& data, unsigned int indentationLength = 2 )
{
    Terathon::Array<char> text;

    if ( data.Write( text, indentationLength ) != Json::Status::kOk )
    {
        return "(not written)";
    }

    text.AppendArrayElement( 0 );

    return Terathon::String<>( text.begin() );
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
