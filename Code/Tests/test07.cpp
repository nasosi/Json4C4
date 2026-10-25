#include "TestCheck.h"

#include <string>

std::string NestedArrays( int depth, bool closed )
{
    return std::string( size_t( depth ), '[' ) + ( closed ? std::string( size_t( depth ), ']' ) : std::string() );
}

std::string NestedObjects( int depth, bool closed )
{
    std::string text;

    for ( int a = 0; a < depth; a++ )
    {
        text += "{\"a\":";
    }

    text += "1";

    if ( closed )
    {
        text += std::string( size_t( depth ), '}' );
    }

    return text;
}

int main()
{
    {
        Json::StructuredData sd;

        CheckStatus( ParseText( sd, NestedArrays( 256, true ).c_str() ).status, Json::Status::kOk, "256 nested arrays" );
        Check( sd.GetRootJsonValue() != nullptr, "256 nested arrays have a root" );
        CheckStatus( sd.Write( "Data/Test/out_test07.json" ), Json::Status::kOk, "write 256 nested arrays" );

        Json::StructuredData reparsed;
        CheckStatus( reparsed.Parse( "Data/Test/out_test07.json" ).status, Json::Status::kOk, "reparse 256 nested arrays" );
    }

    {
        Json::StructuredData sd;

        Json::ParseResult parseResult = ParseText( sd, NestedArrays( 257, true ).c_str() );
        CheckStatus( parseResult.status, Json::Status::kMaximumNestingDepthExceeded, "257 nested arrays" );
        Check( parseResult.errorLine == 1 && parseResult.errorColumn == 257, "257 nested arrays error position" );
        Check( sd.GetRootJsonValue() == nullptr, "257 nested arrays have no root" );
    }

    {
        Json::StructuredData sd;

        CheckStatus( ParseText( sd, NestedArrays( 100000, false ).c_str() ).status, Json::Status::kMaximumNestingDepthExceeded, "100000 open arrays" );
    }

    {
        Json::ParseOptions   options;
        Json::StructuredData sd;

        options.maxNestingDepth = 10;
        CheckStatus( ParseText( sd, NestedArrays( 10, true ).c_str(), options ).status, Json::Status::kOk, "10 nested arrays with a limit of 10" );
        CheckStatus( ParseText( sd, NestedObjects( 11, true ).c_str(), options ).status,
                     Json::Status::kMaximumNestingDepthExceeded,
                     "11 nested objects with a limit of 10" );

        options.maxNestingDepth = 400;
        CheckStatus( ParseText( sd, NestedArrays( 400, true ).c_str(), options ).status, Json::Status::kOk, "400 nested arrays with a limit of 400" );

        options.maxNestingDepth = 0;
        CheckStatus( ParseText( sd, "[]", options ).status, Json::Status::kMaximumNestingDepthExceeded, "a limit of 0 refuses containers" );
        CheckStatus( ParseText( sd, "1", options ).status, Json::Status::kOk, "a limit of 0 accepts scalars" );
    }

    {
        Json::StructuredData sd;

        CheckStatus( ParseText( sd, NestedObjects( 256, true ).c_str() ).status, Json::Status::kOk, "256 nested objects" );
        CheckStatus( ParseText( sd, NestedObjects( 257, true ).c_str() ).status, Json::Status::kMaximumNestingDepthExceeded, "257 nested objects" );
        CheckStatus( ParseText( sd, NestedObjects( 100000, false ).c_str() ).status, Json::Status::kMaximumNestingDepthExceeded, "100000 open objects" );
    }

    {
        std::string text;

        for ( int a = 0; a < 128; a++ )
        {
            text += "[{\"a\":";
        }

        text += "1";

        for ( int a = 0; a < 128; a++ )
        {
            text += "}]";
        }

        Json::StructuredData sd;

        CheckStatus( ParseText( sd, text.c_str() ).status, Json::Status::kOk, "256 levels of mixed arrays and objects" );
        CheckStatus(
            ParseText( sd, ( "[" + text + "]" ).c_str() ).status, Json::Status::kMaximumNestingDepthExceeded, "257 levels of mixed arrays and objects" );
    }

    {
        std::string text = "[";

        for ( int a = 0; a < 100000; a++ )
        {
            text += "[],";
        }

        text += "[]]";

        Json::StructuredData sd;

        CheckStatus( ParseText( sd, text.c_str() ).status, Json::Status::kOk, "wide shallow array" );
    }

    return TestResult();
}
