#include "TestCheck.h"

struct PositionCase
{
    const char*  text;
    Json::Status status;
    int          line;
    int          column;
};

const PositionCase positionCases[] = {
    { "[1,]", Json::Status::kExpectedJsonValue, 1, 4 },
    { "\n\n  [1,]", Json::Status::kExpectedJsonValue, 3, 6 },
    { "   [1,]", Json::Status::kExpectedJsonValue, 1, 7 },
    { "\r[1,]", Json::Status::kExpectedJsonValue, 2, 4 },
    { "\r\n[1,]", Json::Status::kExpectedJsonValue, 2, 4 },
    { "[\r\r\n\n1,]", Json::Status::kExpectedJsonValue, 4, 3 },
    { "[\"abc\x01\"]", Json::Status::kStringIllegalChar, 1, 6 },
    { "[\"ab\\q\"]", Json::Status::kStringIllegalEscape, 1, 5 },
    { "{\n  \"key\" : \"\xC0\xAF\"\n}", Json::Status::kStringIllegalChar, 2, 12 },
    { "{\"a\" 1}", Json::Status::kExpectedColon, 1, 6 },
    { "[1]x", Json::Status::kExpectedEndOfFile, 1, 4 },
    { "[\n  1,\n  2\n", Json::Status::kExpectedClosingSquareBracketsOrComma, 4, 1 },
    { "\"abc", Json::Status::kPrematureNullTerminator, 1, 5 },
    { "[1, 01]", Json::Status::kNumbersCannotHaveLeadingZeros, 1, 5 },
};

int main()
{
    for ( const PositionCase& positionCase : positionCases )
    {
        Json::StructuredData sd;

        Json::ParseResult parseResult = ParseText( sd, positionCase.text );

        CheckStatus( parseResult.status, positionCase.status, positionCase.text );

        if ( parseResult.errorLine != positionCase.line || parseResult.errorColumn != positionCase.column )
        {
            fprintf( stderr,
                     "FAILED: position for \"%s\": %d:%d, expected %d:%d\n",
                     positionCase.text,
                     parseResult.errorLine,
                     parseResult.errorColumn,
                     positionCase.line,
                     positionCase.column );
            testFailureCount++;
        }
    }

    {
        Json::StructuredData sd;

        Json::ParseResult parseResult = ParseText( sd, "[1]" );
        Check( parseResult.errorLine == 0 && parseResult.errorColumn == 0, "no position after a successful parse" );
    }

    return TestResult();
}
