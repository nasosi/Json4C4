#include "TestCheck.h"

#include <clocale>

int main()
{
    const char* locales[] = { "de_DE.UTF-8", "de_DE.utf8", "de_DE", "fr_FR.UTF-8", "fr_FR.utf8", "German_Germany.1252", "German" };

    const char* selected = nullptr;

    for ( const char* locale : locales )
    {
        if ( setlocale( LC_NUMERIC, locale ) )
        {
            selected = locale;
            break;
        }
    }

    if ( !selected )
    {
        fprintf( stdout, "No locale with a decimal comma is installed; skipped\n" );
        return testSkippedReturnCode;
    }

    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "[1.5,1e-3,-2.5E2]" ).status, Json::Status::kOk, "parse under a decimal comma locale" );

    Terathon::Array<double> numbers;
    CheckStatus( sd.DeserializeTo( numbers ), Json::Status::kOk, "deserialize under a decimal comma locale" );
    Check( numbers.GetArrayElementCount() == 3 && numbers[ 0 ] == 1.5 && numbers[ 1 ] == 1e-3 && numbers[ 2 ] == -250, "numbers under a decimal comma locale" );

    numbers[ 0 ] = 2.25;
    CheckStatus( sd.SerializeFrom( numbers ), Json::Status::kOk, "serialize under a decimal comma locale" );
    CheckStatus( sd.Write( "Data/Test/out_test17_locale.json" ), Json::Status::kOk, "write under a decimal comma locale" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test17_locale.json", text ) && strstr( text, "2.25" ) != nullptr && strchr( text, ',' ) == strstr( text, ",\n" ),
           "numbers are written with a decimal point" );

    setlocale( LC_NUMERIC, "C" );

    return TestResult();
}
