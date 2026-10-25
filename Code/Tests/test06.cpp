#include "TestCheck.h"

int main()
{
    constexpr unsigned statusCount = unsigned( Json::Status::kFileWriteError ) + 1;

    for ( unsigned a = 0; a < statusCount; a++ )
    {
        Terathon::String<256> first = Json::StatusToString( Json::Status( a ) );

        Check( strcmp( first, "Unknown status" ) != 0, "every status has a string" );

        for ( unsigned b = 0; b < a; b++ )
        {
            Check( strcmp( first, Json::StatusToString( Json::Status( b ) ) ) != 0, "status strings are distinct" );
        }
    }

    Check( strcmp( Json::StatusToString( Json::Status::kFileTooLarge ), "File is too large" ) == 0, "kFileTooLarge string" );
    Check( strcmp( Json::StatusToString( Json::Status::kStringIllegalEscape ), "Illegal escape" ) == 0, "kStringIllegalEscape string" );
    Check( strcmp( Json::StatusToString( Json::Status::kNameNotPresent ), "Could not find requested name of name/value pair" ) == 0, "kNameNotPresent string" );
    Check( strcmp( Json::StatusToString( Json::Status::kFileWriteError ), "Could not write file" ) == 0, "kFileWriteError string" );
    Check( strcmp( Json::StatusToString( Json::Status( statusCount ) ), "Unknown status" ) == 0, "out of range status" );
    Check( strcmp( Json::StatusToString( Json::Status( 0xFFFFFFFF ) ), "Unknown status" ) == 0, "large out of range status" );

    Json::ParseResult parseResult { Json::Status::kExpectedEndOfFile, 202, 1501 };
    Check( strcmp( Json::ParseResultToString( parseResult ), "Expected end of file. Line: 202, column: 1501" ) == 0,
           "ParseResultToString keeps the whole location" );

    parseResult = { Json::Status::kExpectedJsonValue, 2147483647, 2147483647 };
    Check( strcmp( Json::ParseResultToString( parseResult ),
                   "Expected a JSON value (object, array, string, number, 'true', 'false', or 'null'). Line: 2147483647, column: 2147483647" ) == 0,
           "ParseResultToString with the longest message" );

    return TestResult();
}
