// Floats: a float is written in the shortest form that reads back as the same float, keeps that form through edits and
// copies, and reads back exactly, up to FLT_MAX.

#include "TestCheck.h"

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

using Terathon::Array;
using Terathon::String;

struct Colour
{
    float                  red   = 0;
    float                  green = 0;
    double                 alpha = 0;
    Array<float>           weights;
    std::vector<float>     factors;
    Json::ObjectMap<float> named;
};

#define COLOUR_PROTO                                                                                                                                           \
    "red", object.red, "green", object.green, "alpha", object.alpha, "weights", object.weights, "factors", object.factors, "named", object.named
DEFINE_JSON4C4_FUNCTIONS( Colour, COLOUR_PROTO )

const char* const colourText = "{ \"red\": 0.1, \"green\": 0.33333334, \"alpha\": 0.10000000149011612, \"weights\": [ 3, -0, 3.4028235e+38, 1e-45 ], "
                               "\"factors\": [ 0.2, 16777216, 30000000000, 1e+21, -1e-7 ], \"named\": { \"gain\": 0.3 } }";

void MakeColour( Colour& colour )
{
    const float weights[] = { 3, -0.0f, FLT_MAX, std::numeric_limits<float>::denorm_min() };
    const float factors[] = { 0.2f, 16777216, 3e10f, 1e21f, -1e-7f };

    colour.red   = 0.1f;
    colour.green = 1.0f / 3;
    colour.alpha = 0.1f;

    for ( float weight : weights )
    {
        colour.weights.AppendArrayElement( weight );
    }

    colour.factors.assign( factors, factors + 5 );

    auto* element = new Json::ObjectMap<float>::ElementType;

    element->name = "gain";
    element->data = 0.3f;
    colour.named.InsertMapElement( element );
}

uint32_t Bits( float value )
{
    uint32_t bits;
    memcpy( &bits, &value, sizeof( bits ) );

    return bits;
}

float FromBits( uint32_t bits )
{
    float value;
    memcpy( &value, &bits, sizeof( value ) );

    return value;
}

bool SameColour( const Colour& left, const Colour& right )
{
    bool same = ( Bits( left.red ) == Bits( right.red ) ) && ( Bits( left.green ) == Bits( right.green ) ) && ( left.alpha == right.alpha ) &&
                ( left.weights.GetArrayElementCount() == right.weights.GetArrayElementCount() ) && ( left.factors.size() == right.factors.size() ) &&
                right.named.FindMapElement( "gain" ) && ( Bits( right.named.FindMapElement( "gain" )->data ) == Bits( 0.3f ) );

    for ( int a = 0; same && ( a < left.weights.GetArrayElementCount() ); a++ )
    {
        same = Bits( left.weights[ a ] ) == Bits( right.weights[ a ] );
    }

    for ( size_t a = 0; same && ( a < left.factors.size() ); a++ )
    {
        same = Bits( left.factors[ a ] ) == Bits( right.factors[ a ] );
    }

    return same;
}

String<> WriteNew( const Json::StructuredData& data )
{
    Terathon::Array<char> text;

    if ( data.Write( text ) != Json::Status::kOk )
    {
        return "(not written)";
    }

    text.AppendArrayElement( 0 );

    return String<>( text.begin() );
}

// Whether a document writes the same text as the one parsed from expected.
bool Holds( const Json::StructuredData& data, const char* expected )
{
    Json::StructuredData other;

    return ( ParseText( other, expected ).status == Json::Status::kOk ) && ( strcmp( WriteNew( data ), WriteNew( other ) ) == 0 );
}

// Whether each number of a built array is an integer exactly when it is one once its text is parsed again.
bool SameIntegers( const Json::ArrayView& built, const Json::ArrayView& parsed )
{
    bool same = built.GetElementCount() == parsed.GetElementCount();

    for ( Terathon::uint32 a = 0; same && ( a < built.GetElementCount() ); a++ )
    {
        same = built[ a ].IsInteger() == parsed[ a ].IsInteger();
    }

    return same;
}

void TestWritten()
{
    Colour               colour;
    Colour               back;
    Json::StructuredData data;
    Json::StructuredData parsed;

    MakeColour( colour );

    CheckStatus( data.SerializeFrom( colour ), Json::Status::kOk, "serialize floats" );
    Check( Holds( data, colourText ), "floats are written in their shortest float form" );
    CheckStatus( ParseText( parsed, WriteNew( data ) ).status, Json::Status::kOk, "parse the written floats" );
    CheckStatus( parsed.DeserializeTo( back ), Json::Status::kOk, "read the floats back" );
    Check( SameColour( colour, back ), "every float reads back as the same float" );

    Json::ObjectView built  = data.GetRoot()->AsObject();
    Json::ObjectView reread = parsed.GetRoot()->AsObject();

    Check( built.Find( "red" )->IsFloat() && !built.Find( "alpha" )->IsFloat(), "a number built from a float is marked" );
    Check( !reread.Find( "red" )->IsFloat(), "a parsed number is not marked" );
    Check( built.FindArray( "weights" )[ 0 ].IsInteger() && !built.FindArray( "factors" )[ 3 ].IsInteger(), "3 is an integer and 1e21 is not" );
    Check( SameIntegers( built.FindArray( "weights" ), reread.FindArray( "weights" ) ) &&
               SameIntegers( built.FindArray( "factors" ), reread.FindArray( "factors" ) ),
           "a float is an integer exactly when its text is" );
}

// Edits move values, and the mark moves with them.
void TestEdits()
{
    Json::StructuredData data;
    std::vector<float>   list = { 0.7f };

    CheckStatus( ParseText( data, "{ \"a\": 1 }" ).status, Json::Status::kOk, "parse an object" );

    const Json::Value& root = *data.GetRoot();

    CheckStatus( data.SerializeAt( *root.AsObject().Find( "a" ), 2.5f ), Json::Status::kOk, "replace a number with a float" );
    Check( root.AsObject().Find( "a" )->IsFloat(), "a replaced number is marked" );
    CheckStatus( data.AddMember( root, "f", 0.1f ), Json::Status::kOk, "add a float" );
    CheckStatus( data.AddMember( root, "list", list ), Json::Status::kOk, "add an array of floats" );
    CheckStatus( data.AppendElement( *root.AsObject().Find( "list" ), 0.9f ), Json::Status::kOk, "append a float" );

    String<> expected = "{ \"f\": 0.1, \"list\": [ 0.7, 0.9 ]";

    for ( int a = 0; a < 20; a++ )
    {
        char name[ 16 ];
        char member[ 64 ];

        snprintf( name, sizeof( name ), "k%d", a );
        snprintf( member, sizeof( member ), ", \"%s\": %g", name, a / 4.0 );
        expected += member;

        CheckStatus( data.AddMember( root, name, a / 4.0f ), Json::Status::kOk, "add a member" );
    }

    CheckStatus( data.AddMember( root, "copy", *root.AsObject().Find( "f" ) ), Json::Status::kOk, "copy a float" );
    CheckStatus( data.RemoveMember( root, "a" ), Json::Status::kOk, "remove the first member" );
    expected += ", \"copy\": 0.1 }";

    Check( Holds( data, expected ), "floats keep their form when they move" );

    Json::StructuredData other;

    CheckStatus( other.SerializeFrom( root ), Json::Status::kOk, "copy the document" );
    Check( Holds( other, expected ), "floats keep their form in a copy" );
}

void TestRefused()
{
    Json::StructuredData data;
    float                single = 1;
    const double         limit  = 0x1.ffffffp127;

    CheckStatus( data.SerializeFrom( std::numeric_limits<float>::quiet_NaN() ), Json::Status::kFloatInvalid, "a NaN float" );
    CheckStatus( data.SerializeFrom( -std::numeric_limits<float>::infinity() ), Json::Status::kFloatInvalid, "an infinite float" );

    // Doubles below FLT_MAX and half a step read as FLT_MAX, and from there on they are refused.
    CheckStatus( data.SerializeFrom( std::nextafter( limit, 0.0 ) ), Json::Status::kOk, "the largest double below the float limit" );
    CheckStatus( Json::Deserialize( *data.GetRoot(), single ), Json::Status::kOk, "read the largest double below the float limit" );
    Check( single == FLT_MAX, "the largest double below the float limit reads as FLT_MAX" );

    CheckStatus( data.SerializeFrom( -std::nextafter( limit, 0.0 ) ), Json::Status::kOk, "the smallest double above minus the float limit" );
    CheckStatus( Json::Deserialize( *data.GetRoot(), single ), Json::Status::kOk, "read the smallest double above minus the float limit" );
    Check( single == -FLT_MAX, "the smallest double above minus the float limit reads as -FLT_MAX" );

    CheckStatus( data.SerializeFrom( limit ), Json::Status::kOk, "the float limit" );
    CheckStatus( Json::Validate( *data.GetRoot(), single ), Json::Status::kFloatInvalid, "validate the float limit" );
    CheckStatus( Json::Deserialize( *data.GetRoot(), single ), Json::Status::kFloatInvalid, "read the float limit" );
    Check( single == -FLT_MAX, "a refused float is left as it was" );

    CheckStatus( ParseText( data, "[ -1e39 ]" ).status, Json::Status::kOk, "parse a number beyond any float" );
    CheckStatus( Json::Deserialize( data.GetRoot()->AsArray()[ 0 ], single ), Json::Status::kFloatInvalid, "read a number beyond any float" );
    CheckStatus( ParseText( data, "\"text\"" ).status, Json::Status::kOk, "parse a string" );
    CheckStatus( Json::Deserialize( *data.GetRoot(), single ), Json::Status::kInvalidValueType, "read a string as a float" );

    Colour           colour;
    const char*      text   = "{ \"red\": 0.5, \"green\": 1e39 }";
    Json::ReadResult result = Json::Read( text, strlen( text ), colour );

    CheckStatus( result.status, Json::Status::kFloatInvalid, "read a structure with a float out of range" );
    Check( ( result.path == "green" ) && ( result.errorLine == 1 ) && ( result.errorColumn == 24 ), "the path and position of a float out of range" );
}

// A sample of floats from every binade, written and read back through documents.
void TestSample()
{
    std::vector<float> floats;

    // 7.038531e-26 and its negative, whose shortest form reads back as a double that narrows to another float.
    const uint32_t doubles[] = { 0x15AE43FDu, 0x95AE43FDu };

    for ( uint64_t pattern = 0; pattern <= UINT32_MAX; pattern += 65537 )
    {
        if ( ( pattern & 0x7F800000u ) != 0x7F800000u )
        {
            floats.push_back( FromBits( uint32_t( pattern ) ) );
        }
    }

    for ( uint32_t bits : doubles )
    {
        floats.push_back( FromBits( bits ) );
    }

    Json::StructuredData data;
    Json::StructuredData parsed;
    std::vector<float>   back;

    CheckStatus( data.SerializeFrom( floats ), Json::Status::kOk, "serialize a sample of floats" );
    CheckStatus( ParseText( parsed, WriteNew( data ) ).status, Json::Status::kOk, "parse a sample of floats" );
    CheckStatus( parsed.DeserializeTo( back ), Json::Status::kOk, "read a sample of floats back" );

    bool same = back.size() == floats.size();

    for ( size_t a = 0; same && ( a < floats.size() ); a++ )
    {
        same = Bits( back[ a ] ) == Bits( floats[ a ] );
    }

    Check( same, "every float of the sample reads back as the same float" );
    Check( SameIntegers( data.GetRoot()->AsArray(), parsed.GetRoot()->AsArray() ), "every float of the sample is an integer exactly when its text is" );
}

int main()
{
    TestWritten();
    TestEdits();
    TestRefused();
    TestSample();

    return TestResult();
}
