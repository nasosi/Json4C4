#include "TestCheck.h"

struct ConformanceCase
{
    const char*  text;
    size_t       length;
    Json::Status status;
};

#define CASE( text, status ) { text, sizeof( text ) - 1, Json::Status::status }

const ConformanceCase conformanceCases[] = {
    CASE( "0", kOk ),
    CASE( "-0", kOk ),
    CASE( "5", kOk ),
    CASE( " 7 ", kOk ),
    CASE( "0e1", kOk ),
    CASE( "0E-2", kOk ),
    CASE( "-0.5e+10", kOk ),
    CASE( "[1.5,1e5,1E+5,1e-5,-12.25]", kOk ),
    CASE( "\"\\u00e9\"", kOk ),
    CASE( "\"\\uD83D\\uDE00\"", kOk ),
    CASE( "\"\x7F\"", kOk ),
    CASE( "\"\xC3\xA9\"", kOk ),
    CASE( "\"\xED\x9F\xBF\"", kOk ),
    CASE( "\"\xEE\x80\x80\"", kOk ),
    CASE( "\"\xF0\x9F\x98\x80\"", kOk ),
    CASE( "\"\xF4\x8F\xBF\xBF\"", kOk ),
    CASE( "\"\\/\\b\\f\\n\\r\\t\\\"\\\\\"", kOk ),
    CASE( "{\"a\":1}", kOk ),
    CASE( " [ ] ", kOk ),
    CASE( "{ }", kOk ),
    CASE( "true", kOk ),
    CASE( "null", kOk ),
    CASE( "01", kNumbersCannotHaveLeadingZeros ),
    CASE( "-01", kNumbersCannotHaveLeadingZeros ),
    CASE( "[00]", kNumbersCannotHaveLeadingZeros ),
    CASE( "[1-2]", kExpectedClosingSquareBracketsOrComma ),
    CASE( "[1.2.3]", kExpectedClosingSquareBracketsOrComma ),
    CASE( "[1e5e5]", kExpectedClosingSquareBracketsOrComma ),
    CASE( "[0x10]", kExpectedClosingSquareBracketsOrComma ),
    CASE( "[--1]", kFloatInvalid ),
    CASE( "[-+1]", kFloatInvalid ),
    CASE( "[- 1]", kFloatInvalid ),
    CASE( "[-]", kFloatInvalid ),
    CASE( "[1.]", kFloatInvalid ),
    CASE( "[1.e5]", kFloatInvalid ),
    CASE( "[1e]", kFloatInvalid ),
    CASE( "[1e+]", kFloatInvalid ),
    CASE( "[-Infinity]", kFloatInvalid ),
    CASE( "[+1]", kExpectedJsonValue ),
    CASE( "[.5]", kExpectedJsonValue ),
    CASE( "[Infinity]", kExpectedJsonValue ),
    CASE( "[NaN]", kExpectedJsonValue ),
    CASE( "\"\\u0000\"", kStringIllegalEscape ),
    CASE( "\"\\uD83D\"", kStringIllegalEscape ),
    CASE( "\"\\uDE00\"", kStringIllegalEscape ),
    CASE( "\"\\uDE00\\uD83D\"", kStringIllegalEscape ),
    CASE( "\"\\uD83D\\u0041\"", kStringIllegalEscape ),
    CASE( "\"\\uD83Dx\"", kStringIllegalEscape ),
    CASE( "\"\\u12\"", kStringIllegalEscape ),
    CASE( "\"\\x\"", kStringIllegalEscape ),
    CASE( "\"\xED\xA0\x80\"", kStringIllegalChar ),
    CASE( "\"\xED\xBF\xBF\"", kStringIllegalChar ),
    CASE( "\"\xC0\xAF\"", kStringIllegalChar ),
    CASE( "\"\xE0\x80\xAF\"", kStringIllegalChar ),
    CASE( "\"\xF4\x90\x80\x80\"", kStringIllegalChar ),
    CASE( "\"\xC3\"", kStringIllegalChar ),
    CASE( "\"\x1F\"", kStringIllegalChar ),
    CASE( "\"abc", kPrematureNullTerminator ),
    CASE( "[tru]", kExpectedTrueOrFalse ),
    CASE( "[nul]", kExpectedNull ),
    CASE( "[1,]", kExpectedJsonValue ),
    CASE( "[,1]", kExpectedJsonValue ),
    CASE( "{\"a\":1,}", kExpectedJsonValue ),
    CASE( "{\"a\" 1}", kExpectedColon ),
    CASE( "{1:1}", kExpectedBeginingDoubleQuotes ),
    CASE( "[1", kExpectedClosingSquareBracketsOrComma ),
    CASE( "{\"a\":1", kExpectedClosingCurlyBracesOrComma ),
    CASE( " ", kExpectedJsonValue ),
    CASE( "[1] [2]", kExpectedEndOfFile ),
    CASE( "[1]\0[2]", kExpectedEndOfFile ),
    CASE( "[1]\0", kExpectedEndOfFile ),
    CASE( "\"a\0b\"", kStringIllegalChar ),
    CASE( "", kTextBufferContentsInvalid ),
};

const double* RootNumber( const Json::StructuredData& sd )
{
    return sd.GetRootJsonValue() ? sd.GetRootJsonValue()->GetDataAsPointerTo<double>() : nullptr;
}

const Terathon::String<>* RootString( const Json::StructuredData& sd )
{
    return sd.GetRootJsonValue() ? sd.GetRootJsonValue()->GetDataAsPointerTo<Terathon::String<>>() : nullptr;
}

int main()
{
    for ( const ConformanceCase& conformanceCase : conformanceCases )
    {
        Json::StructuredData sd;

        Json::ParseResult parseResult = ParseText( sd, conformanceCase.text, conformanceCase.length );

        CheckStatus( parseResult.status, conformanceCase.status, conformanceCase.text );
        Check( ( parseResult.status == Json::Status::kOk ) == ( sd.GetRootJsonValue() != nullptr ), "a root exists only after a successful parse" );
    }

    {
        Json::StructuredData sd;

        ParseText( sd, "5" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == 5.0, "single digit root" );

        ParseText( sd, "-0.5e+10" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == -5e9, "negative number with exponent" );

        ParseText( sd, "0.1" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == 0.1, "0.1" );

        ParseText( sd, "1.00000000000000000000000000000000000000000000000000000000000000000000000000001" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == 1.0, "number longer than the local buffer" );

        ParseText( sd, "10000000000000000000000000000000000000000000000000000000000000000000000000000000" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == 1e79, "every digit of a long number counts" );

        ParseText( sd, "123456789012345678" );
        Check( RootNumber( sd ) && *RootNumber( sd ) == 123456789012345678.0, "large integer" );

        ParseText( sd, "\"\\uD83D\\uDE00\"" );
        Check( RootString( sd ) && strcmp( *RootString( sd ), "\xF0\x9F\x98\x80" ) == 0, "escaped surrogate pair becomes four byte UTF-8" );
        Check( RootString( sd ) && RootString( sd )->GetStringLength() == 4, "escaped surrogate pair length" );

        ParseText( sd, "\"a\\u00e9\\u20AC\\uD834\\uDD1Eb\"" );
        Check( RootString( sd ) && strcmp( *RootString( sd ),
                                           "a\xC3\xA9\xE2\x82\xAC\xF0\x9D\x84\x9E"
                                           "b" ) == 0,
               "mixed escapes" );

        ParseText( sd, "\"\x7F\"" );
        Check( RootString( sd ) && strcmp( *RootString( sd ), "\x7F" ) == 0, "raw DEL" );
    }

    return TestResult();
}
