// Reading into structures: Validate and DeserializeTo give what 2.3.0 gave for the same structures and texts, and Read reports
// the path and the position of the value at fault.

#include "TestCheck.h"

#include <map>
#include <string>
#include <vector>

using Terathon::Array;
using Terathon::String;

struct Extension
{
    String<> name;
    double   version = 0;
};

#define EXTENSION_PROTO "name", object.name, "version", object.version
DEFINE_JSON4C4_FUNCTIONS( Extension, EXTENSION_PROTO )

struct Settings
{
    bool   enabled = false;
    double scale   = 0;

#define SETTINGS_PROTO "enabled", enabled, Json::optional, "scale", scale
    DEFINE_JSON4C4_MEMBER_FUNCTIONS( SETTINGS_PROTO )
};

struct Layer
{
    String<>                            name;
    Array<Extension>                    extensions;
    Json::ObjectMap<String<>>           functions;
    Array<Array<double>>                matrix;
    Settings                            settings;
    Json::Null                          nothing;
    std::string                         label;
    std::vector<std::string>            tags;
    std::map<std::string, double>       weights;
    std::vector<std::vector<Extension>> groups;
};

#define LAYER_PROTO                                                                                                                                            \
    "name", object.name, "extensions", object.extensions, "functions", object.functions, Json::optional, "matrix", object.matrix, "settings", object.settings, \
        "nothing", object.nothing, Json::optional, "label", object.label, Json::optional, "tags", object.tags, Json::optional, "weights", object.weights,      \
        Json::optional, "groups", object.groups
DEFINE_JSON4C4_FUNCTIONS( Layer, LAYER_PROTO )

struct Manifest
{
    String<> version;
    Layer    layer;
};

#define MANIFEST_PROTO "file_format_version", object.version, "layer", object.layer
DEFINE_JSON4C4_FUNCTIONS( Manifest, MANIFEST_PROTO )

const char* const valid = R"({
  "file_format_version": "1.0.0",
  "layer": {
    "name": "XR_layer",
    "extensions": [ { "name": "XR_EXT_a", "version": 4 }, { "name": "XR_EXT_b", "version": 1.5 } ],
    "functions": { "xrGet": "Get", "xrNegotiate": "Negotiate" },
    "matrix": [ [ 1, 0 ], [ 0, 1 ] ],
    "settings": { "enabled": true },
    "nothing": null,
    "label": "first",
    "tags": [ "a", "b", "c" ],
    "weights": { "x": 1, "y": 2.5 },
    "groups": [ [ { "name": "g", "version": 7 } ], [] ]
  }
})";

// Writes what a manifest serializes to, so that two manifests can be compared.
String<> WriteManifest( const Manifest& manifest )
{
    Json::StructuredData  data;
    Terathon::Array<char> text;

    if ( ( data.SerializeFrom( manifest ) != Json::Status::kOk ) || ( data.Write( text ) != Json::Status::kOk ) )
    {
        return "(not serialized)";
    }

    text.AppendArrayElement( 0 );

    return String<>( text.begin() );
}

// The results of reading the manifests, to check against what 2.3.0 gave.
ResultHash results;

// Validates and deserializes a text, and adds the statuses and the manifest read to the results.
void AddResults( const char* text )
{
    Json::StructuredData data;
    Manifest             manifest;
    Json::Status         parse = data.ParseText( text, strlen( text ) ).status;

    results.Add( parse );

    if ( parse != Json::Status::kOk )
    {
        return;
    }

    Json::Status validate    = Json::Validate( data, manifest );
    Json::Status deserialize = data.DeserializeTo( manifest );

    results.Add( validate );
    results.Add( deserialize );

    if ( deserialize == Json::Status::kOk )
    {
        String<> written = WriteManifest( manifest );
        results.Add( static_cast<const char*>( written ), size_t( written.GetStringLength() ) );
    }
}

// The valid manifest with one piece of text replaced.
String<> Replace( const char* from, const char* to )
{
    String<>    text  = valid;
    const char* found = strstr( valid, from );

    if ( found )
    {
        text = String<>( valid, Terathon::int32( found - valid ) );
        text += to;
        text += found + strlen( from );
    }

    return text;
}

void TestSameAs2x()
{
    struct Case
    {
        const char* from;
        const char* to;
        const char* description;
    };

    static const Case cases[] = {
        { "", "", "a valid manifest" },
        { "\"label\": \"first\",", "", "an optional member left out" },
        { "\"name\": \"XR_layer\",", "", "a required member left out" },
        { "\"version\": 1.5", "\"version\": \"1.5\"", "a string for a number in an array element" },
        { "[ 0, 1 ]", "[ 0, true ]", "a bool for a number in an array of arrays" },
        { "\"Negotiate\"", "7", "a number for a string in a map" },
        { "\"enabled\": true", "\"enabled\": 1", "a number for a bool in a member structure" },
        { "\"nothing\": null", "\"nothing\": 0", "a number for null" },
        { "\"tags\": [ \"a\", \"b\", \"c\" ]", "\"tags\": [ \"a\", 2 ]", "a number in a vector of strings" },
        { "\"y\": 2.5", "\"y\": \"2.5\"", "a string in a map of numbers" },
        { "\"version\": 7", "\"version\": null", "null in a vector of vectors" },
        { "\"extensions\": [ { \"name\": \"XR_EXT_a\", \"version\": 4 }, { \"name\": \"XR_EXT_b\", \"version\": 1.5 } ]",
          "\"extensions\": { \"a\": 1 }",
          "an object for an array" },
        { "\"functions\": { \"xrGet\": \"Get\", \"xrNegotiate\": \"Negotiate\" }", "\"functions\": [ 1 ]", "an array for a map" },
    };

    for ( const Case& c : cases )
    {
        AddResults( Replace( c.from, c.to ) );
    }

    AddResults( "{ \"file_format_version\": \"1\", \"layer\": [] }" );
    AddResults( "[ 1 ]" );

    CheckHash( results.Take(), 0x213AEAF2385B0095ULL, "the manifests read as 2.3.0 read them" );
}

// Reads a manifest and checks the status and path, and that the position is where marker begins, offset by offset.
void CheckRead( const char*          text,
                Json::Status         status,
                const char*          path,
                const char*          marker,
                int                  offset,
                const char*          description,
                Json::DuplicateNames duplicateNames = Json::DuplicateNames::kReject )
{
    Manifest           manifest;
    Json::ParseOptions options;

    options.duplicateNames = duplicateNames;

    Json::ReadResult result = Json::Read( text, strlen( text ), manifest, options );

    int line   = 0;
    int column = 0;

    if ( marker )
    {
        const char* position  = strstr( text, marker ) + offset;
        const char* lineStart = text;

        line = 1;

        for ( const char* c = text; c != position; c++ )
        {
            if ( c[ 0 ] == '\n' )
            {
                line++;
                lineStart = c + 1;
            }
        }

        column = int( position - lineStart ) + 1;
    }

    bool same = ( result.status == status ) && ( strcmp( result.path, path ) == 0 ) && ( result.errorLine == line ) && ( result.errorColumn == column );

    if ( !same )
    {
        fprintf( stderr,
                 "%s: status %u, path \"%s\" at %d:%d, expected %d:%d\n",
                 description,
                 unsigned( result.status ),
                 static_cast<const char*>( result.path ),
                 int( result.errorLine ),
                 int( result.errorColumn ),
                 line,
                 column );
    }

    Check( same, description );
}

void TestRead()
{
    CheckRead( valid, Json::Status::kOk, "", nullptr, 0, "read a manifest" );

    String<> text = Replace( "\"name\": \"XR_layer\",", "" );
    CheckRead( text, Json::Status::kNameNotPresent, "layer.name", "\"layer\": {", 9, "a missing member is reported at its object" );

    text = Replace( "\"version\": 1.5", "\"version\": \"1.5\"" );
    CheckRead(
        text, Json::Status::kInvalidValueType, "layer.extensions[1].version", "\"version\": \"1.5\"", 11, "a value of the wrong type in an array element" );

    text = Replace( "[ 0, 1 ]", "[ 0, true ]" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.matrix[1][1]", "[ 0, true ]", 5, "an element of an array of arrays" );

    text = Replace( "\"Negotiate\"", "7" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.functions.xrNegotiate", "\"xrNegotiate\": 7", 15, "a value in a map" );

    text = Replace( "\"enabled\": true", "\"scale\": 2" );
    CheckRead(
        text, Json::Status::kNameNotPresent, "layer.settings.enabled", "\"settings\": {", 12, "a member missing from a structure with member functions" );

    text = Replace( "\"enabled\": true", "\"enabled\": true, \"scale\": \"2\"" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.settings.scale", "\"scale\": \"2\"", 9, "an optional member of the wrong type" );

    text = Replace( "\"version\": 7", "\"version\": null" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.groups[0][0].version", "\"version\": null", 11, "a vector of vectors" );

    text = Replace( "\"y\": 2.5", "\"y\": \"2.5\"" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.weights.y", "\"y\": \"2.5\"", 5, "a map of numbers" );

    text = Replace( "\"first\"", "\"fi\\u0000rst\"" );
    CheckRead( text, Json::Status::kOk, "", nullptr, 0, "a std::string may hold a zero" );

    text = Replace( "\"XR_layer\"", "\"XR\\u0000layer\"" );
    CheckRead( text, Json::Status::kStringHoldsZero, "layer.name", "\"name\": \"XR", 8, "a String<> may not hold a zero" );

    text = Replace( "\"xrGet\"", "\"xr\\u0000Get\"" );
    CheckRead( text, Json::Status::kStringHoldsZero, "layer.functions.xr\\u0000Get", "\"xr\\u0000Get\": \"Get\"", 15, "a map name with a zero" );

    text = Replace( "\"version\": 1.5", "\"vers\\u0069on\": \"1.5\"" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.extensions[1].version", "\"vers\\u0069on\": \"1.5\"", 16, "a name written with an escape" );

    text = Replace( "\"label\": \"first\"", "\"label\": \"first\", \"label\": 2" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.label", "\"label\": 2", 9, "the last of a repeated name", Json::DuplicateNames::kKeepLast );

    text = Replace( "\"label\": \"first\"", "\"label\": 2, \"label\": \"first\"" );
    CheckRead( text, Json::Status::kInvalidValueType, "layer.label", "\"label\": 2", 9, "the first of a repeated name", Json::DuplicateNames::kKeepFirst );

    CheckRead( "[ 1 ]", Json::Status::kInvalidValueType, "", "[", 0, "the root of the wrong type" );
    CheckRead( "{ \"file_format_version\": 1 ", Json::Status::kExpectedClosingCurlyBracesOrComma, "", "{", 27, "a text that does not parse" );

    Manifest manifest;
    CheckStatus( Json::Read( "Data/Examples/missing.json", manifest ).status, Json::Status::kFileOpenError, "read a missing file" );
}

struct Holder
{
    Settings settings;
};

#define HOLDER_PROTO Json::optional, "settings", object.settings
DEFINE_JSON4C4_FUNCTIONS( Holder, HOLDER_PROTO )

// An optional member that is present has to be complete, where 2.x let it pass.
void TestOptionalMembers()
{
    const char* text = "{ \"settings\": { \"scale\": 2 } }";

    Holder holder;

    Json::ReadResult result = Json::Read( text, strlen( text ), holder );
    CheckStatus( result.status, Json::Status::kNameNotPresent, "an incomplete optional member" );
    Check( strcmp( result.path, "settings.enabled" ) == 0, "an incomplete optional member is reported" );

    CheckStatus( Json::Read( "{}", 2, holder ).status, Json::Status::kOk, "an optional member left out" );
}

int main()
{
    TestSameAs2x();
    TestRead();
    TestOptionalMembers();

    return TestResult();
}
