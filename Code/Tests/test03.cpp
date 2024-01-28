#include <Json4C4/C4Json.h>

namespace Json = C4::Json;

const char* mustFail[] = { "Data/Test/jsonchecker/fail2.json",  "Data/Test/jsonchecker/fail3.json",  "Data/Test/jsonchecker/fail4.json",
                           "Data/Test/jsonchecker/fail5.json",  "Data/Test/jsonchecker/fail6.json",  "Data/Test/jsonchecker/fail7.json",
                           "Data/Test/jsonchecker/fail8.json",  "Data/Test/jsonchecker/fail9.json",  "Data/Test/jsonchecker/fail10.json",
                           "Data/Test/jsonchecker/fail11.json", "Data/Test/jsonchecker/fail12.json", "Data/Test/jsonchecker/fail13.json",
                           "Data/Test/jsonchecker/fail14.json", "Data/Test/jsonchecker/fail15.json", "Data/Test/jsonchecker/fail16.json",
                           "Data/Test/jsonchecker/fail17.json", "Data/Test/jsonchecker/fail19.json", "Data/Test/jsonchecker/fail20.json",
                           "Data/Test/jsonchecker/fail21.json", "Data/Test/jsonchecker/fail22.json", "Data/Test/jsonchecker/fail23.json",
                           "Data/Test/jsonchecker/fail24.json", "Data/Test/jsonchecker/fail25.json", "Data/Test/jsonchecker/fail26.json",
                           "Data/Test/jsonchecker/fail27.json", "Data/Test/jsonchecker/fail28.json", "Data/Test/jsonchecker/fail29.json",
                           "Data/Test/jsonchecker/fail30.json", "Data/Test/jsonchecker/fail31.json", "Data/Test/jsonchecker/fail32.json" };

const char* mustPass[] = { "Data/Test/jsonchecker/fail1_PASSES_RFC_8259.json",
                           "Data/Test/jsonchecker/fail18_PASSES_RFC_8259.json",
                           "Data/Test/jsonchecker/pass1.json",
                           "Data/Test/jsonchecker/pass2.json",
                           "Data/Test/jsonchecker/pass3.json" };

int main()
{
    for ( auto a = 0; a != 30; a++ )
    {
        Json::StructuredData jSd;

        auto parseResult = jSd.Parse( mustFail[ a ] );

        if ( parseResult.status == Json::Status::KFileOpenError || parseResult.status == Json::Status::KFileReadError )
        {
            fprintf( stdout, "File error for file: %s", mustFail[ a ] );
        }
        else if ( parseResult.status != Json::Status::kOk )
        {
            Terathon::String<256> errorDescription = Json::ParseResultToString( parseResult );

            fprintf( stdout, "JSON parse error (expected): %s for file %s\n", static_cast<const char*>( errorDescription ), mustFail[ a ] );
        }
        else
        {
            fprintf( stdout, "Parsing of %s was expected to fail but it didn't", mustFail[ a ] );
            return 1;
        }
    }

    for ( auto a = 0; a != 5; a++ )
    {
        Json::StructuredData jSd;

        auto parseResult = jSd.Parse( mustPass[ a ] );

        if ( parseResult.status != Json::Status::kOk )
        {
            Terathon::String<256> errorDescription = Json::ParseResultToString( parseResult );

            fprintf( stdout, "JSON parse error: %s for file %s\n", static_cast<const char*>( errorDescription ), mustPass[ a ] );
            return 1;
        }
    }

    return 0;
}