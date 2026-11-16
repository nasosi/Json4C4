#include "TestCheck.h"

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    include <cstdlib>
#    include <map>
#    include <string>
#    include <vector>

namespace V3 = Json::V3;

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

V3::ObjectView RootObject( const V3::StructuredData& sd )
{
    return sd.GetRoot() ? sd.GetRoot()->AsObject() : V3::ObjectView();
}

void TestEscaping()
{
    V3::StructuredData sd;

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

    V3::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_escaping.json" ).status, Json::Status::kOk, "reparse escaped strings" );

    V3::ObjectView root = RootObject( reparsed );
    Check( root && root.FindBool( "admin" ) == nullptr, "no injected member" );
    Check( root && root.GetMemberCount() == 3, "three members after the round trip" );

    Strings roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize reparsed strings" );
    Check( roundTrip.name == original.name && roundTrip.path == original.path && roundTrip.control == original.control, "strings survive the round trip" );

    V3::StructuredData keys;
    CheckStatus( ParseText( keys, "{\"a\\\"b\\\\c\\n\":1}" ).status, Json::Status::kOk, "parse a key that needs escaping" );
    CheckStatus( keys.Write( "Data/Test/out_test10_keys.json" ), Json::Status::kOk, "write a key that needs escaping" );
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_keys.json" ).status, Json::Status::kOk, "reparse a key that needs escaping" );
    Check( RootObject( reparsed ).FindNumber( "a\"b\\c\n" ) != nullptr, "escaped key survives the round trip" );
}

// Infinities and NaN fail when they are serialized. A number that overflows when it is parsed fails when it is written.
void TestNonFinite()
{
    volatile double huge = 1e308;

    Number number;
    number.value = huge * 10.0;

    V3::StructuredData sd;
    CheckStatus( sd.SerializeFrom( number ), Json::Status::kFloatInvalid, "infinity is not serialized" );

    number.value = huge * 10.0 - huge * 10.0;
    CheckStatus( sd.SerializeFrom( number ), Json::Status::kFloatInvalid, "NaN is not serialized" );

    CheckStatus( WriteTextFile( "Data/Test/out_test10_nonfinite.json", "{\"value\":1}" ) ? Json::Status::kOk : Json::Status::kFileWriteError,
                 Json::Status::kOk,
                 "write the file to keep" );

    CheckStatus( ParseText( sd, "[1e400]" ).status, Json::Status::kOk, "parse an overflowing number" );
    CheckStatus( sd.Write( "Data/Test/out_test10_nonfinite.json" ), Json::Status::kFloatInvalid, "an overflowing number is not written" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test10_nonfinite.json", text ) && text == "{\"value\":1}", "a failed write leaves the old file" );
}

void TestNoRoot()
{
    remove( "Data/Test/out_test10_noroot.json" );

    V3::StructuredData sd;
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

    V3::StructuredData sd;
    CheckStatus( sd.SerializeFrom( maps ), Json::Status::kOk, "serialize maps" );
    CheckStatus( sd.Write( "Data/Test/out_test10_maps.json" ), Json::Status::kOk, "write maps" );

    V3::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_maps.json" ).status, Json::Status::kOk, "reparse maps" );

    V3::ObjectView root = RootObject( reparsed );
    V3::ObjectView m    = root.FindObject( "m" );
    V3::ObjectView sm   = root.FindObject( "sm" );

    Check( m.FindNumber( "x" ) && *m.FindNumber( "x" ) == 42, "ObjectMap member is written" );
    Check( sm.FindNumber( "y" ) && *sm.FindNumber( "y" ) == 7, "std::map member is written" );

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

    V3::StructuredData sd;
    CheckStatus( sd.SerializeFrom( vectors ), Json::Status::kOk, "serialize vectors" );
    CheckStatus( sd.Write( "Data/Test/out_test10_vectors.json" ), Json::Status::kOk, "write vectors" );

    V3::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test10_vectors.json" ).status, Json::Status::kOk, "reparse vectors" );

    V3::ObjectView root = RootObject( reparsed );
    Check( root.FindArray( "linespec" ).GetElementCount() == 3, "vector member is an array" );

    Vectors roundTrip;
    CheckStatus( reparsed.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize vectors" );
    Check( roundTrip.linespec == vectors.linespec && roundTrip.rows == vectors.rows && roundTrip.names == vectors.names, "vectors survive the round trip" );

    std::vector<double> rootVector = { 4, 5 };
    CheckStatus( sd.SerializeFrom( rootVector ), Json::Status::kOk, "serialize a vector root" );
    Check( sd.GetRoot() && sd.GetRoot()->AsArray(), "a vector root is an array" );

    std::vector<double> rootRoundTrip;
    CheckStatus( sd.DeserializeTo( rootRoundTrip ), Json::Status::kOk, "deserialize a vector root" );
    Check( rootRoundTrip == rootVector, "vector root survives the round trip" );
}

void TestOverwrite()
{
    V3::StructuredData sd;

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":\"long string value that makes the first file long\"}" ).status, Json::Status::kOk, "parse the long document" );
    CheckStatus( sd.Write( "Data/Test/out_test10_overwrite.json" ), Json::Status::kOk, "write the long document" );

    CheckStatus( ParseText( sd, "{\"a\":1}" ).status, Json::Status::kOk, "parse the short document" );
    CheckStatus( sd.Write( "Data/Test/out_test10_overwrite.json" ), Json::Status::kOk, "write the short document" );

    V3::StructuredData reparsed;
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

    V3::StructuredData sd;
    CheckStatus( sd.SerializeFrom( numbers ), Json::Status::kOk, "serialize numbers" );
    CheckStatus( sd.Write( "Data/Test/out_test10_numbers.json" ), Json::Status::kOk, "write numbers" );

    V3::StructuredData reparsed;
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

void TestNumberLayout()
{
    struct Layout
    {
        double      value;
        const char* text;
    };

    const Layout layouts[] = { { 0.1, "0.1" },
                               { 123456789, "123456789" },
                               { -0.0, "-0" },
                               { 0.000001, "0.000001" },
                               { 1e-7, "1e-7" },
                               { 1e20, "100000000000000000000" },
                               { 1e21, "1e+21" },
                               { 5e-324, "5e-324" },
                               { 1.7976931348623157e308, "1.7976931348623157e+308" } };

    Terathon::Array<double> numbers;

    for ( const Layout& layout : layouts )
    {
        numbers.AppendArrayElement( layout.value );
    }

    V3::StructuredData sd;
    CheckStatus( sd.SerializeFrom( numbers ), Json::Status::kOk, "serialize the layout numbers" );
    CheckStatus( sd.Write( "Data/Test/out_test10_layout.json" ), Json::Status::kOk, "write the layout numbers" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test10_layout.json", text ), "read the layout numbers" );

    for ( const Layout& layout : layouts )
    {
        Terathon::String<> element = "\n  ";
        element += layout.text;

        Terathon::String<> followedByComma = element;
        followedByComma += ",";

        Terathon::String<> followedByNewline = element;
        followedByNewline += "\n";

        Check( strstr( text, followedByComma ) != nullptr || strstr( text, followedByNewline ) != nullptr, layout.text );
    }
}

void TestInvalidStrings()
{
    V3::StructuredData sd;
    CheckStatus( ParseText( sd, "[\"text\"]" ).status, Json::Status::kOk, "parse a string" );

    Check( WriteTextFile( "Data/Test/out_test10_invalid.json", "[1]" ), "write the file to keep" );

    const V3::Value& string = sd.GetRoot()->AsArray()[ 0 ];

    const char* invalid[] = { "\xFF", "a\xC3", "\xED\xA0\x80", "\xC0\xAF", "\xF4\x90\x80\x80" };

    for ( const char* text : invalid )
    {
        CheckStatus( sd.SerializeAt( string, Terathon::String<>( text ) ), Json::Status::kOk, "replace a string with invalid UTF-8" );
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

    CheckStatus( sd.SerializeAt( string, Terathon::String<>( "\xC3\xA9\xF0\x9F\x98\x80" ) ), Json::Status::kOk, "replace a string with valid UTF-8" );
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

    V3::StructuredData sd;
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
    TestNumberLayout();
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
