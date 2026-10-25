#include "TestCheck.h"

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    include <cstdlib>
#    include <map>
#    include <string>
#    include <vector>

struct Strings
{
    Terathon::String<> name;
    Terathon::String<> path;
    Terathon::String<> control;
};

#    define STRINGS_PROTO "name", object.name, "path", object.path, "control", object.control
DEFINE_JSON4C4_FUNCTIONS( Strings, STRINGS_PROTO )

struct Maps
{
    double                        v = 1;
    Json::ObjectMap<double>       m;
    std::map<std::string, double> sm;
};

#    define MAPS_PROTO "v", object.v, "m", object.m, "sm", object.sm
DEFINE_JSON4C4_FUNCTIONS( Maps, MAPS_PROTO )

struct Vectors
{
    std::vector<double>              linespec;
    std::vector<std::vector<double>> rows;
    std::vector<std::string>         names;
};

#    define VECTORS_PROTO "linespec", object.linespec, "rows", object.rows, "names", object.names
DEFINE_JSON4C4_FUNCTIONS( Vectors, VECTORS_PROTO )

struct Number
{
    double value = 0;
};

#    define NUMBER_PROTO "value", object.value
DEFINE_JSON4C4_FUNCTIONS( Number, NUMBER_PROTO )

const Json::ObjectValue* RootObject( const Json::StructuredData& sd )
{
    return sd.GetRootJsonValue() ? sd.GetRootJsonValue()->AsJsonObjectValue() : nullptr;
}

void TestEscaping()
{
    Json::StructuredData sd;

    CheckStatus(
        ParseText(
            sd,
            "{\"name\":\"x\\\",\\\"admin\\\":true,\\\"pad\\\":\\\"y\",\"path\":\"C:\\\\dir\\\\new\",\"control\":\"\\u0001\\b\\f\\n\\r\\t\\\"\\\\/\\u001f\"}" )
            .status,
        Json::Status::kOk,
        "parse strings that need escaping" );

    Strings original;
    CheckStatus( sd.DeserializeTo( original ), Json::Status::kOk, "deserialize strings" );
    Check( strcmp( original.name, "x\",\"admin\":true,\"pad\":\"y" ) == 0, "decoded quotes" );
    Check( strcmp( original.path, "C:\\dir\\new" ) == 0, "decoded backslashes" );
    Check( strcmp( original.control, "\x01\b\f\n\r\t\"\\/\x1F" ) == 0, "decoded control characters" );

    CheckStatus( sd.Write( "Data/Test/out_test10_escaping.json" ), Json::Status::kOk, "write strings" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_escaping.json" ).status, Json::Status::kOk, "reparse escaped strings" );

    const Json::ObjectValue* root = RootObject( reparsed );
    Check( root && root->FindBool( "admin" ) == nullptr, "no injected member" );
    Check( root && root->GetMapElementCount() == 3, "three members after the round trip" );

    Strings roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize reparsed strings" );
    Check( roundTrip.name == original.name && roundTrip.path == original.path && roundTrip.control == original.control, "strings survive the round trip" );

    Json::StructuredData keys;
    CheckStatus( ParseText( keys, "{\"a\\\"b\\\\c\\n\":1}" ).status, Json::Status::kOk, "parse a key that needs escaping" );
    CheckStatus( keys.Write( "Data/Test/out_test10_keys.json" ), Json::Status::kOk, "write a key that needs escaping" );
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_keys.json" ).status, Json::Status::kOk, "reparse a key that needs escaping" );
    Check( RootObject( reparsed ) && RootObject( reparsed )->FindNumber( "a\"b\\c\n" ) != nullptr, "escaped key survives the round trip" );
}

void TestNonFinite()
{
    volatile double huge = 1e308;

    Number number;
    number.value = huge * 10.0;

    Json::StructuredData sd;
    CheckStatus( sd.SerializeFrom( number ), Json::Status::kOk, "serialize infinity" );

    CheckStatus( WriteTextFile( "Data/Test/out_test10_nonfinite.json", "{\"value\":1}" ) ? Json::Status::kOk : Json::Status::kFileWriteError,
                 Json::Status::kOk,
                 "write the file to keep" );
    CheckStatus( sd.Write( "Data/Test/out_test10_nonfinite.json" ), Json::Status::kFloatInvalid, "infinity is not written" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test10_nonfinite.json", text ) && text == "{\"value\":1}", "a failed write leaves the old file" );

    number.value = huge * 10.0 - huge * 10.0;
    CheckStatus( sd.SerializeFrom( number ), Json::Status::kOk, "serialize NaN" );
    CheckStatus( sd.Write( "Data/Test/out_test10_nonfinite.json" ), Json::Status::kFloatInvalid, "NaN is not written" );

    CheckStatus( ParseText( sd, "[1e400]" ).status, Json::Status::kOk, "parse an overflowing number" );
    CheckStatus( sd.Write( "Data/Test/out_test10_nonfinite.json" ), Json::Status::kFloatInvalid, "an overflowing number is not written" );
}

void TestNoRoot()
{
    remove( "Data/Test/out_test10_noroot.json" );

    Json::StructuredData sd;
    CheckStatus( sd.Write( "Data/Test/out_test10_noroot.json" ), Json::Status::kInvalidStructuredData, "write without a root" );

    FILE* file = fopen( "Data/Test/out_test10_noroot.json", "rb" );
    Check( file == nullptr, "write without a root does not create the file" );

    if ( file )
    {
        fclose( file );
    }
}

void TestMaps()
{
    Maps maps;

    auto* element = new Json::ObjectMap<double>::ElementType;
    element->name = "x";
    element->data = 42;
    maps.m.InsertMapElement( element );
    maps.sm[ "y" ] = 7;

    Json::StructuredData sd;
    CheckStatus( sd.SerializeFrom( maps ), Json::Status::kOk, "serialize maps" );
    CheckStatus( sd.Write( "Data/Test/out_test10_maps.json" ), Json::Status::kOk, "write maps" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_maps.json" ).status, Json::Status::kOk, "reparse maps" );

    const Json::ObjectValue* root = RootObject( reparsed );
    const Json::ObjectValue* m    = root ? root->FindJsonObjectValue( "m" ) : nullptr;
    const Json::ObjectValue* sm   = root ? root->FindJsonObjectValue( "sm" ) : nullptr;

    Check( m && m->FindNumber( "x" ) && *m->FindNumber( "x" ) == 42, "ObjectMap member is written" );
    Check( sm && sm->FindNumber( "y" ) && *sm->FindNumber( "y" ) == 7, "std::map member is written" );

    Maps roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize maps" );
    Check( roundTrip.m.FindMapElement( "x" ) && roundTrip.m.FindMapElement( "x" )->data == 42 && roundTrip.sm[ "y" ] == 7, "maps survive the round trip" );
}

void TestVectors()
{
    Vectors vectors;
    vectors.linespec = { 1, 2.5, 3 };
    vectors.rows     = { { 1, 2 }, {}, { 3 } };
    vectors.names    = { "a", "b\"c" };

    Json::StructuredData sd;
    CheckStatus( sd.SerializeFrom( vectors ), Json::Status::kOk, "serialize vectors" );
    CheckStatus( sd.Write( "Data/Test/out_test10_vectors.json" ), Json::Status::kOk, "write vectors" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_vectors.json" ).status, Json::Status::kOk, "reparse vectors" );

    const Json::ObjectValue* root = RootObject( reparsed );
    Check( root && root->FindJsonValueArray( "linespec" ) && root->FindJsonValueArray( "linespec" )->GetArrayElementCount() == 3, "vector member is an array" );

    Vectors roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize vectors" );
    Check( roundTrip.linespec == vectors.linespec && roundTrip.rows == vectors.rows && roundTrip.names == vectors.names, "vectors survive the round trip" );

    std::vector<double> rootVector = { 4, 5 };
    CheckStatus( sd.SerializeFrom( rootVector ), Json::Status::kOk, "serialize a vector root" );
    Check( sd.GetRootJsonValue() && sd.GetRootJsonValue()->GetDataAsPointerTo<Terathon::Array<Json::Value*>>() != nullptr, "a vector root is an array" );

    std::vector<double> rootRoundTrip;
    CheckStatus( sd.DeserializeTo( rootRoundTrip ), Json::Status::kOk, "deserialize a vector root" );
    Check( rootRoundTrip == rootVector, "vector root survives the round trip" );
}

void TestOverwrite()
{
    Json::StructuredData sd;

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":\"long string value that makes the first file long\"}" ).status, Json::Status::kOk, "parse the long document" );
    CheckStatus( sd.Write( "Data/Test/out_test10_overwrite.json" ), Json::Status::kOk, "write the long document" );

    CheckStatus( ParseText( sd, "{\"a\":1}" ).status, Json::Status::kOk, "parse the short document" );
    CheckStatus( sd.Write( "Data/Test/out_test10_overwrite.json" ), Json::Status::kOk, "write the short document" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_overwrite.json" ).status, Json::Status::kOk, "overwriting with a shorter document truncates the file" );
}

void TestNumbers()
{
    Terathon::Array<double> numbers;

    const double values[] = { 0.1, 1e300, -1e-300, 123456789, 5e-324, 0.30000000000000004, -0.0, 1.7976931348623157e308, 2.2250738585072014e-308, 1.0 / 3.0 };

    for ( double value : values )
    {
        numbers.AppendArrayElement( value );
    }

    Json::StructuredData sd;
    CheckStatus( sd.SerializeFrom( numbers ), Json::Status::kOk, "serialize numbers" );
    CheckStatus( sd.Write( "Data/Test/out_test10_numbers.json" ), Json::Status::kOk, "write numbers" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_numbers.json" ).status, Json::Status::kOk, "reparse numbers" );

    Terathon::Array<double> roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize numbers" );
    Check( roundTrip.GetArrayElementCount() == numbers.GetArrayElementCount(), "number count survives the round trip" );

    for ( int a = 0; a < roundTrip.GetArrayElementCount() && a < numbers.GetArrayElementCount(); a++ )
    {
        Check( memcmp( &roundTrip[ a ], &numbers[ a ], sizeof( double ) ) == 0, "number survives the round trip exactly" );
    }

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test10_numbers.json", text ) && strstr( text, "  0.1,\n" ) != nullptr, "0.1 is written in its shortest form" );
}

void TestInvalidStrings()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "[\"text\"]" ).status, Json::Status::kOk, "parse a string" );

    Check( WriteTextFile( "Data/Test/out_test10_invalid.json", "[1]" ), "write the file to keep" );

    Terathon::String<>* string =
        sd.GetRootJsonValue()->GetDataAsPointerTo<Terathon::Array<Json::Value*>>()->begin()[ 0 ]->GetDataAsPointerTo<Terathon::String<>>();

    const char* invalid[] = { "\xFF", "a\xC3", "\xED\xA0\x80", "\xC0\xAF", "\xF4\x90\x80\x80" };

    for ( const char* text : invalid )
    {
        *string = text;
        CheckStatus( sd.Write( "Data/Test/out_test10_invalid.json" ), Json::Status::kStringIllegalChar, "invalid UTF-8 is not written" );
    }

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test10_invalid.json", text ) && text == "[1]", "a refused write leaves the old file" );

    FILE* temporary = fopen( "Data/Test/out_test10_invalid.json.json4c4.tmp", "rb" );
    Check( temporary == nullptr, "no temporary file is left behind" );

    if ( temporary )
    {
        fclose( temporary );
    }

    *string = "\xC3\xA9\xF0\x9F\x98\x80";
    CheckStatus( sd.Write( "Data/Test/out_test10_invalid.json" ), Json::Status::kOk, "valid UTF-8 is written" );
    Check( ReadTextFile( "Data/Test/out_test10_invalid.json", text ) && strstr( text, "\xC3\xA9\xF0\x9F\x98\x80" ) != nullptr,
           "valid UTF-8 is written unchanged" );
}

void TestLargeFile()
{
    std::string text = "[";

    for ( int a = 0; a < 20000; a++ )
    {
        text += "12345,";
    }

    text += "0]";

    Check( WriteTextFile( "Data/Test/out_test10_large.json", text.c_str() ), "write the large file" );

    Json::StructuredData sd;
    CheckStatus( sd.Parse( "Data/Test/out_test10_large.json" ).status, Json::Status::kOk, "parse a file over 64 KB" );

    Terathon::Array<double> numbers;
    CheckStatus( sd.DeserializeTo( numbers ), Json::Status::kOk, "deserialize the large file" );
    Check( numbers.GetArrayElementCount() == 20001, "element count of the large file" );
}

int main()
{
    TestEscaping();
    TestNonFinite();
    TestNoRoot();
    TestMaps();
    TestVectors();
    TestOverwrite();
    TestNumbers();
    TestInvalidStrings();
    TestLargeFile();

    return TestResult();
}

#else

int main()
{
    return testSkippedReturnCode;
}

#endif
