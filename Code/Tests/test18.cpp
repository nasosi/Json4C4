#include "TestCheck.h"

#include <cstdlib>

bool SameResult( Json::StructuredData& first, const Json::ParseResult& firstResult, Json::StructuredData& second, const Json::ParseResult& secondResult )
{
    if ( ( firstResult.status != secondResult.status ) || ( firstResult.errorLine != secondResult.errorLine ) ||
         ( firstResult.errorColumn != secondResult.errorColumn ) )
    {
        return false;
    }

    if ( firstResult.status != Json::Status::kOk )
    {
        return true;
    }

    Terathon::Array<char> firstText;
    Terathon::Array<char> secondText;
    Terathon::Array<char> indentation;

    first.GetRootJsonValue()->Write( firstText, 2, ' ', indentation );
    second.GetRootJsonValue()->Write( secondText, 2, ' ', indentation );

    return ( firstText.GetArrayElementCount() == secondText.GetArrayElementCount() ) &&
           ( memcmp( firstText.begin(), secondText.begin(), size_t( firstText.GetArrayElementCount() ) ) == 0 );
}

int shownMismatches = 0;

// Parses the first length characters of text from an exact-size heap buffer, so that a read past the end is a read past
// the allocation, and checks that the same characters give the same result when the text goes on after them and when
// they end in a terminating zero.
void CheckBounded( const char* text, size_t length, const char* description )
{
    char* exact = static_cast<char*>( malloc( ( length != 0 ) ? length : 1 ) );
    memcpy( exact, text, length );

    Json::StructuredData bounded;
    Json::StructuredData followed;
    Json::StructuredData terminated;
    Json::ParseResult    boundedResult    = bounded.ParseText( exact, length );
    Json::ParseResult    followedResult   = followed.ParseText( text, length );
    Json::ParseResult    terminatedResult = ParseText( terminated, text, length );

    bool same = SameResult( bounded, boundedResult, followed, followedResult ) && SameResult( bounded, boundedResult, terminated, terminatedResult );

    if ( !same && ( shownMismatches++ < 10 ) )
    {
        fprintf( stderr,
                 "%s, length %u: status %u at %d:%d, followed by more text %u at %d:%d\n",
                 description,
                 unsigned( length ),
                 unsigned( boundedResult.status ),
                 int( boundedResult.errorLine ),
                 int( boundedResult.errorColumn ),
                 unsigned( followedResult.status ),
                 int( followedResult.errorLine ),
                 int( followedResult.errorColumn ) );
    }

    Check( same, description );

    free( exact );
}

void CheckEveryPrefix( const char* text, size_t length, const char* description )
{
    for ( size_t a = 0; a <= length; a++ )
    {
        CheckBounded( text, a, description );
    }
}

void TestDataFiles()
{
    static const char* const files[] = {
        "Data/Examples/car.json",
        "Data/Examples/simple.json",
        "Data/Examples/simple_error.json",
        "Data/Examples/simple_with_array.json",
        "Data/Test/test_file01.json",
        "Data/Test/test_file02.json",
        "Data/Test/test_file04.json",
        "Data/Test/jsonchecker/pass1.json",
        "Data/Test/jsonchecker/pass2.json",
        "Data/Test/jsonchecker/pass3.json",
        "Data/Test/jsonchecker/fail1_PASSES_RFC_8259.json",
        "Data/Test/jsonchecker/fail18_PASSES_RFC_8259.json",
    };

    for ( const char* file : files )
    {
        Terathon::String<> text;
        Check( ReadTextFile( file, text ), file );
        CheckEveryPrefix( text, size_t( text.GetStringLength() ), file );
    }

    char name[ 64 ];

    for ( int a = 2; a <= 33; a++ )
    {
        snprintf( name, sizeof( name ), "Data/Test/jsonchecker/fail%d.json", a );

        Terathon::String<> text;

        if ( ( a != 18 ) && ReadTextFile( name, text ) )
        {
            CheckEveryPrefix( text, size_t( text.GetStringLength() ), name );
        }
    }
}

// Every token that can end at the end of the text, each followed by every character that could continue it.
void TestTokensAtTheEnd()
{
    static const char* const tokens[] = { "true",
                                          "false",
                                          "null",
                                          "\"abc\"",
                                          "\"\\n\\/\\\"\"",
                                          "\"\\u00e9\"",
                                          "\"\\ud83d\\ude00\"",
                                          "\"\xC3\xA9\"",
                                          "\"\xE2\x82\xAC\"",
                                          "\"\xF0\x9F\x98\x80\"",
                                          "-12.5e+3",
                                          "0",
                                          "[1,2]",
                                          "{\"a\":1}",
                                          "  \r\n  1  \r" };

    static const char* const followers[] = { "e", "0", "\"", "\\", "u0041\"", "\\ude00\"", "\x80\x80\x80", "}", "]", "\n", "x" };

    for ( const char* token : tokens )
    {
        for ( const char* follower : followers )
        {
            Terathon::String<> text = token;
            text += follower;

            for ( size_t a = 0; a <= strlen( token ); a++ )
            {
                CheckBounded( text, a, token );
            }
        }
    }
}

// Texts whose ends lie far from the characters that end whitespace and string runs.
void TestLongTails()
{
    Terathon::String<> letters;
    Terathon::String<> spaces;

    for ( int a = 0; a < 5000; a++ )
    {
        letters += "a";
        spaces += " ";
    }

    struct Case
    {
        Terathon::String<> text;
        Json::Status       status;
    };

    Case cases[] = {
        { Terathon::String<>( "[\"" ) + letters, Json::Status::kPrematureNullTerminator },
        { Terathon::String<>( "[\"x\",\"" ) + letters, Json::Status::kPrematureNullTerminator },
        { Terathon::String<>( "[\"" ) + letters + "\"]", Json::Status::kOk },
        { Terathon::String<>( "[\"" ) + letters + "\\u00e9", Json::Status::kPrematureNullTerminator },
        { Terathon::String<>( "[1]" ) + spaces, Json::Status::kOk },
        { spaces + "[1]", Json::Status::kOk },
        { spaces, Json::Status::kExpectedJsonValue },
        { Terathon::String<>( "[1," ) + spaces, Json::Status::kExpectedJsonValue },
    };

    for ( const Case& c : cases )
    {
        size_t length = size_t( c.text.GetStringLength() );
        char*  exact  = static_cast<char*>( malloc( length ) );
        memcpy( exact, c.text, length );

        Json::StructuredData sd;
        CheckStatus( sd.ParseText( exact, length ).status, c.status, "a text with a long tail" );
        CheckBounded( c.text, length, "a text with a long tail" );

        free( exact );
    }
}

void TestEdges()
{
    Json::StructuredData sd;

    CheckStatus( sd.ParseText( "[1]", 3 ).status, Json::Status::kOk, "a text without a terminating zero" );
    CheckStatus( sd.ParseText( nullptr, 5 ).status, Json::Status::kTextBufferContentsInvalid, "no text" );
    Check( sd.GetRootJsonValue() == nullptr, "a failed parse clears the previous document" );
    CheckStatus( sd.ParseText( "[1]", 0 ).status, Json::Status::kTextBufferContentsInvalid, "an empty text" );
    CheckStatus( sd.ParseText( "[1]", Terathon::uint64( 0x7FFFFFFFU ) ).status, Json::Status::kFileTooLarge, "a text one byte longer than the limit" );
    CheckStatus( sd.ParseText( "[1]", Terathon::uint64( 0x80000000U ) ).status, Json::Status::kFileTooLarge, "a text longer than the limit" );
    CheckStatus( sd.ParseText( "[1]", ~Terathon::uint64( 0 ) ).status, Json::Status::kFileTooLarge, "the longest length" );

    Json::ParseResult result = sd.ParseText( "[\"a\0b\"]", 7 );
    CheckStatus( result.status, Json::Status::kStringIllegalChar, "a zero inside a string" );
    Check( ( result.errorLine == 1 ) && ( result.errorColumn == 4 ), "a zero inside a string is found where it is" );

    CheckStatus( sd.ParseText( "[1]\0", 4 ).status, Json::Status::kExpectedEndOfFile, "a zero after the value" );
    CheckStatus( sd.ParseText( "\"ab", 3 ).status, Json::Status::kPrematureNullTerminator, "a string cut by the end" );
}

// Calls written for 2.2 keep their meaning next to ParseText.
void TestParseOverloads()
{
    Json::StructuredData  sd;
    Terathon::Array<char> buffer = MakeTextBuffer( "[1]", 3 );

    CheckStatus( sd.Parse( "Data/Examples/simple.json", {} ).status, Json::Status::kOk, "a file with braced options" );
    Check( sd.GetRootJsonValue() && sd.GetRootJsonValue()->AsJsonObjectValue(), "a file with braced options is read" );
    CheckStatus( sd.Parse( buffer, {} ).status, Json::Status::kOk, "a buffer with braced options" );
    CheckStatus( sd.Parse( "Data/Examples/simple.json" ).status, Json::Status::kOk, "a file without options" );
    CheckStatus( sd.Parse( buffer ).status, Json::Status::kOk, "a buffer without options" );
}

void TestLimits()
{
    Json::StructuredData sd;
    Json::ParseOptions   options;

    options.maxValueCount = 4;
    CheckStatus( ParseText( sd, "[1,2,3]", options ).status, Json::Status::kOk, "as many values as the limit" );

    options.maxValueCount    = 3;
    Json::ParseResult result = ParseText( sd, "[1,2,3]", options );
    CheckStatus( result.status, Json::Status::kMaximumValueCountExceeded, "one value more than the limit" );
    Check( ( result.errorLine == 1 ) && ( result.errorColumn == 6 ) && ( sd.GetRootJsonValue() == nullptr ), "the value past the limit is reported" );

    options.maxValueCount = 0;
    CheckStatus( ParseText( sd, "1", options ).status, Json::Status::kMaximumValueCountExceeded, "a limit of no values" );

    const char* strings = "{\"ab\":\"cd\",\"e\":\"\\u00e9\"}";

    options                    = Json::ParseOptions();
    options.maxStringByteCount = 7;
    CheckStatus( ParseText( sd, strings, options ).status, Json::Status::kOk, "as many string bytes as the limit" );

    options.maxStringByteCount = 6;
    result                     = ParseText( sd, strings, options );
    CheckStatus( result.status, Json::Status::kMaximumStringByteCountExceeded, "one string byte more than the limit" );
    Check( ( result.errorLine == 1 ) && ( result.errorColumn == 16 ) && ( sd.GetRootJsonValue() == nullptr ), "the string past the limit is reported" );

    options.maxStringByteCount = 4;
    CheckStatus( ParseText( sd, strings, options ).status, Json::Status::kMaximumStringByteCountExceeded, "names count as string bytes" );

    options               = Json::ParseOptions();
    options.maxValueCount = -1;
    CheckStatus( ParseText( sd, "1", options ).status, Json::Status::kMaximumValueCountExceeded, "a negative value limit allows no value" );

    options                    = Json::ParseOptions();
    options.maxStringByteCount = -1;
    CheckStatus( ParseText( sd, "[\"\"]", options ).status, Json::Status::kMaximumStringByteCountExceeded, "a negative string limit allows no string" );
    CheckStatus( ParseText( sd, "[1]", options ).status, Json::Status::kOk, "a negative string limit allows a document without strings" );
}

int main()
{
    TestDataFiles();
    TestTokensAtTheEnd();
    TestLongTails();
    TestEdges();
    TestParseOverloads();
    TestLimits();

    return TestResult();
}
