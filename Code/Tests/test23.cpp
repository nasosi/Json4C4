// The 3.0 builder: SerializeFrom writes what 2.x writes for the same structures, the document it builds reads back the
// same, every allocation goes through the block source, and repeated names and misuse fail.

#include "TestCheck.h"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace V3 = Json::V3;

using Terathon::Array;
using Terathon::String;

// Hands out blocks with malloc, refuses the requests from refusedFirst to refusedLast, and checks that every block comes
// back once with its size.
class CountingSource final : public V3::BlockSource
{
public:
    int64_t requestCount     = 0;
    int64_t refusedFirst     = -1;
    int64_t refusedLast      = -1;
    int64_t outstandingBytes = 0;
    int     outstandingCount = 0;

    void* AllocateBlock( Terathon::uint64 size ) noexcept override
    {
        int64_t request = requestCount++;

        if ( ( request >= refusedFirst ) && ( request <= refusedLast ) )
        {
            return nullptr;
        }

        outstandingBytes += int64_t( size );
        outstandingCount++;

        return malloc( size_t( size ) );
    }

    void FreeBlock( void* block, Terathon::uint64 size ) noexcept override
    {
        outstandingBytes -= int64_t( size );
        outstandingCount--;
        free( block );
    }
};

struct Point
{
    double x = 0;
    double y = 0;
};

#define POINT_PROTO "x", object.x, "y", object.y
DEFINE_JSON4C4_FUNCTIONS( Point, POINT_PROTO )

struct Style
{
    bool       visible = false;
    Json::Null nothing;

#define STYLE_PROTO "visible", visible, "nothing", nothing
    DEFINE_JSON4C4_MEMBER_FUNCTIONS( STYLE_PROTO )
};

// A structure without a name in the prototype adds its members to the object around it.
struct Shape
{
    Point                        origin;
    String<>                     name;
    Array<Point>                 points;
    Json::ObjectMap<double>      weights;
    Style                        style;
    std::string                  label;
    std::vector<std::string>     tags;
    std::map<std::string, Point> anchors;
    Array<Array<double>>         matrix;
    std::vector<bool>            flags;
};

#define SHAPE_PROTO                                                                                                                                            \
    object.origin, "name", object.name, "points", object.points, "weights", object.weights, "style", object.style, Json::optional, "label", object.label,      \
        "tags", object.tags, "anchors", object.anchors, "matrix", object.matrix, "flags", object.flags
DEFINE_JSON4C4_FUNCTIONS( Shape, SHAPE_PROTO )

struct Repeated
{
    double a = 1;
    double b = 2;
};

#define REPEATED_PROTO "a", object.a, "b", object.b, "a", object.b
DEFINE_JSON4C4_FUNCTIONS( Repeated, REPEATED_PROTO )

void MakeShape( Shape& shape )
{
    shape.origin = Point { 1.5, -2 };
    shape.name   = "triangle";
    shape.label  = "a \"quoted\" label\n";
    shape.tags   = { "red", "", "\xC3\xA9" };
    shape.flags  = { true, false, true };

    for ( int a = 0; a < 3; a++ )
    {
        shape.points.AppendArrayElement( Point { double( a ), a * 0.25 } );
    }

    const char*  names[]  = { "w1", "w2", "w3", "w4" };
    const double values[] = { 0, 1e21, 1e20, -2.5 };

    for ( int a = 0; a < 4; a++ )
    {
        auto* element = new Json::ObjectMap<double>::ElementType;

        element->name = names[ a ];
        element->data = values[ a ];
        shape.weights.InsertMapElement( element );
    }

    shape.anchors[ "top" ]    = Point { 0, 1 };
    shape.anchors[ "bottom" ] = Point { 0, -1e-7 };

    shape.matrix.SetArrayElementCount( 2 );
    shape.matrix[ 0 ].AppendArrayElement( 1 );
    shape.matrix[ 0 ].AppendArrayElement( 0.1 );
}

String<> WriteOld( const Shape& shape )
{
    Json::StructuredData  sd;
    Terathon::Array<char> text;
    Terathon::Array<char> indentation;

    if ( ( sd.SerializeFrom( shape ) != Json::Status::kOk ) || ( sd.GetRootJsonValue()->Write( text, 2, ' ', indentation ) != Json::Status::kOk ) )
    {
        return "(2.x failed)";
    }

    text.AppendArrayElement( 0 );

    return String<>( text.begin() );
}

String<> WriteNew( const V3::StructuredData& data )
{
    Terathon::Array<char> text;

    if ( data.Write( text ) != Json::Status::kOk )
    {
        return "(3.0 failed)";
    }

    text.AppendArrayElement( 0 );

    return String<>( text.begin() );
}

// Whether two values are the same, down to whether each number is an integer.
bool Same( const V3::Value& left, const V3::Value& right )
{
    if ( ( left.GetType() != right.GetType() ) || ( left.IsInteger() != right.IsInteger() ) )
    {
        return false;
    }

    switch ( left.GetType() )
    {
        case V3::Type::kNull:
            return true;

        case V3::Type::kBool:
            return *left.AsBool() == *right.AsBool();

        case V3::Type::kNumber:
            return *left.AsNumber() == *right.AsNumber();

        case V3::Type::kString:
            return left.AsString() == right.AsString();

        case V3::Type::kArray:
        {
            V3::ArrayView leftArray  = left.AsArray();
            V3::ArrayView rightArray = right.AsArray();
            bool          same       = leftArray.GetElementCount() == rightArray.GetElementCount();

            for ( Terathon::uint32 a = 0; same && ( a < leftArray.GetElementCount() ); a++ )
            {
                same = Same( leftArray.begin()[ a ], rightArray.begin()[ a ] );
            }

            return same;
        }

        case V3::Type::kObject:
        {
            V3::ObjectView leftObject  = left.AsObject();
            V3::ObjectView rightObject = right.AsObject();
            bool           same        = leftObject.GetMemberCount() == rightObject.GetMemberCount();

            for ( Terathon::uint32 a = 0; same && ( a < leftObject.GetMemberCount() ); a++ )
            {
                const V3::Member& leftMember  = leftObject.begin()[ a ];
                const V3::Member& rightMember = rightObject.begin()[ a ];

                same = ( leftMember.GetName() == rightMember.GetName() ) && Same( leftMember.GetValue(), rightMember.GetValue() );
            }

            return same;
        }
    }

    return false;
}

void TestSameAs2x()
{
    Shape              shape;
    V3::StructuredData data;

    MakeShape( shape );
    CheckStatus( data.SerializeFrom( shape ), Json::Status::kOk, "serialize a structure" );

    String<>   written = WriteNew( data );
    ResultHash writtenHash;

    writtenHash.Add( static_cast<const char*>( written ), size_t( written.GetStringLength() ) );
    Check( strcmp( WriteOld( shape ), written ) == 0, "2.x and 3.0 write the same text" );
    CheckHash( writtenHash.Take(), 0x04DF0B9ACCBA6B95ULL, "3.0 writes the structure as 2.3.0 did" );

    // The built document and the one parsed from its text are the same, integers included.
    V3::StructuredData parsed;
    CheckStatus( parsed.ParseText( written, Terathon::uint64( written.GetStringLength() ) ).status, Json::Status::kOk, "parse the written text" );
    Check( strcmp( WriteNew( parsed ), written ) == 0, "the written text reads back the same" );

    Check( data.GetRoot() && parsed.GetRoot() && Same( *data.GetRoot(), *parsed.GetRoot() ), "the built and the parsed documents are the same" );

    V3::ObjectView   built   = data.GetRoot() ? data.GetRoot()->AsObject() : V3::ObjectView();
    V3::ObjectView   weights = built.FindObject( "weights" );
    const V3::Value* x       = built.Find( "x" );
    const V3::Value* y       = built.Find( "y" );
    const V3::Value* large   = weights.Find( "w2" );
    const V3::Value* below   = weights.Find( "w3" );

    Check( x && y && x->AsNumber() && ( *x->AsNumber() == 1.5 ) && !x->IsInteger() && y->IsInteger(), "the members of a structure without a name" );
    Check( large && below && !large->IsInteger() && below->IsInteger(), "a number of 1e21 or more is not an integer" );

    Repeated             repeated;
    Json::StructuredData old;
    CheckStatus( old.SerializeFrom( repeated ), Json::Status::kDuplicateName, "2.x refuses a repeated name in a prototype" );
    CheckStatus( data.SerializeFrom( repeated ), Json::Status::kDuplicateName, "a repeated name in a prototype" );
    Check( !data.GetRoot(), "a failed build leaves the document empty" );
}

// A custom Serialize that writes what it is told, to test the builder's rules.
struct Custom
{
    enum class Kind
    {
        kRepeatedName,
        kElementInObject,
        kMemberWhileInnerOpen,
        kNumberForOpenObject,
        kLargeObject,
        kStatusesIgnored,
        kMisuseIgnored
    };

    Kind kind = Kind::kLargeObject;
};

Json::Status Serialize( V3::Node node, const Custom& custom ) noexcept
{
    Json::Status status = node.SetObject();
    V3::Node     member;
    V3::Node     element;
    char         name[ 16 ];

    switch ( custom.kind )
    {
        case Custom::Kind::kRepeatedName:
            // One name repeated at the end of a large object, which the index finds.
            for ( int a = 0; ( a < 41 ) && ( status == Json::Status::kOk ); a++ )
            {
                snprintf( name, sizeof( name ), "k%d", ( a == 40 ) ? 7 : a );
                status = node.AddMember( name, member );
                status = ( status == Json::Status::kOk ) ? member.SetNumber( a ) : status;
            }

            return status;

        case Custom::Kind::kElementInObject:
            return ( status == Json::Status::kOk ) ? node.AppendElement( element ) : status;

        case Custom::Kind::kMemberWhileInnerOpen:
            status = ( status == Json::Status::kOk ) ? node.AddMember( "inner", member ) : status;
            status = ( status == Json::Status::kOk ) ? member.SetObject() : status;
            return ( status == Json::Status::kOk ) ? node.AddMember( "next", element ) : status;

        case Custom::Kind::kNumberForOpenObject:
            return ( status == Json::Status::kOk ) ? node.SetNumber( 1 ) : status;

        case Custom::Kind::kLargeObject:
            // Forty members, so that the object carries an index.
            for ( int a = 0; ( a < 40 ) && ( status == Json::Status::kOk ); a++ )
            {
                snprintf( name, sizeof( name ), "k%d", a );
                status = node.AddMember( name, member );
                status = ( status == Json::Status::kOk ) ? member.SetNumber( a ) : status;
            }

            return status;

        case Custom::Kind::kStatusesIgnored:
            // The same forty members, with every status ignored and one node used for all of them.
            for ( int a = 0; a < 40; a++ )
            {
                snprintf( name, sizeof( name ), "k%d", a );
                node.AddMember( name, member );
                member.SetNumber( a );
            }

            return Json::Status::kOk;

        case Custom::Kind::kMisuseIgnored:
            node.SetNumber( 1 );
            return Json::Status::kOk;
    }

    return Json::Status::kOk;
}

Json::Status SerializeCustom( V3::StructuredData& data, Custom::Kind kind )
{
    Custom custom;

    custom.kind = kind;

    return data.SerializeFrom( custom );
}

bool HasLargeObject( const V3::StructuredData& data )
{
    V3::ObjectView object = data.GetRoot() ? data.GetRoot()->AsObject() : V3::ObjectView();
    bool           found  = object.GetMemberCount() == 40;

    for ( int a = 0; found && ( a < 40 ); a++ )
    {
        char name[ 16 ];
        snprintf( name, sizeof( name ), "k%d", a );

        found = object.FindNumber( name ) && ( *object.FindNumber( name ) == a );
    }

    return found;
}

void TestRules()
{
    V3::StructuredData data;

    CheckStatus( SerializeCustom( data, Custom::Kind::kRepeatedName ), Json::Status::kDuplicateName, "a repeated name in a large object" );
    CheckStatus( SerializeCustom( data, Custom::Kind::kElementInObject ), Json::Status::kInvalidValueType, "an element added to an object" );
    CheckStatus( SerializeCustom( data, Custom::Kind::kMemberWhileInnerOpen ), Json::Status::kInvalidValueType, "a member added while another object is open" );
    CheckStatus( SerializeCustom( data, Custom::Kind::kNumberForOpenObject ), Json::Status::kInvalidValueType, "a number given to an open object" );
    CheckStatus( SerializeCustom( data, Custom::Kind::kMisuseIgnored ), Json::Status::kInvalidValueType, "a failed call fails the build" );
    Check( !data.GetRoot(), "a failed call leaves the document empty" );

    CheckStatus( SerializeCustom( data, Custom::Kind::kLargeObject ), Json::Status::kOk, "a large object built by hand" );
    Check( HasLargeObject( data ), "every member of a large object built by hand is found" );

    CheckStatus( SerializeCustom( data, Custom::Kind::kStatusesIgnored ), Json::Status::kOk, "a large object built with statuses ignored" );
    Check( HasLargeObject( data ), "every member of a large object built with statuses ignored is found" );

    V3::Node empty;
    CheckStatus( empty.SetNumber( 1 ), Json::Status::kInvalidValueType, "a node that was never given" );
    Check( empty.GetOpenCount() == 0, "a node that was never given has nothing open" );

    Array<double> numbers;
    numbers.AppendArrayElement( 3 );
    CheckStatus( data.SerializeFrom( numbers ), Json::Status::kOk, "an array as the root" );
    Check( data.GetRoot() && ( data.GetRoot()->AsArray().GetElementCount() == 1 ), "the root array" );

    CheckStatus( data.SerializeFrom( 2.5 ), Json::Status::kOk, "a number as the root" );
    Check( data.GetRoot() && data.GetRoot()->AsNumber() && ( *data.GetRoot()->AsNumber() == 2.5 ), "the root number" );

    // A number that cannot be written fails the build, where 2.x fails when the document is written.
    CheckStatus( data.SerializeFrom( std::numeric_limits<double>::quiet_NaN() ), Json::Status::kFloatInvalid, "NaN" );
    CheckStatus( data.SerializeFrom( -std::numeric_limits<double>::infinity() ), Json::Status::kFloatInvalid, "an infinity" );
}

// Refuses each block request of a build in turn, and then every request from it on. The build either gives the same
// document, having asked again at a smaller size, or fails with kAllocationRefused and leaves the document empty, and
// every block comes back.
template <class T>
void CheckRefusals( const T& data, int64_t leastRequests, const char* description )
{
    CountingSource source;
    String<>       expected;
    int64_t        requests;

    {
        V3::StructuredData document( source );

        CheckStatus( document.SerializeFrom( data ), Json::Status::kOk, description );
        expected = WriteNew( document );
        requests = source.requestCount;
    }

    bool closed = ( requests >= leastRequests ) && ( source.outstandingCount == 0 );
    int  failed = 0;

    for ( int64_t refused = 0; closed && ( refused < 2 * requests ); refused++ )
    {
        {
            V3::StructuredData document( source );

            source.requestCount = 0;
            source.refusedFirst = refused % requests;
            source.refusedLast  = ( refused < requests ) ? source.refusedFirst : INT64_MAX;

            Json::Status status = document.SerializeFrom( data );

            closed = ( status == Json::Status::kOk ) ? ( strcmp( WriteNew( document ), expected ) == 0 )
                                                     : ( ( status == Json::Status::kAllocationRefused ) && !document.GetRoot() );
            failed += ( status != Json::Status::kOk ) ? 1 : 0;
        }

        closed = closed && ( source.outstandingCount == 0 ) && ( source.outstandingBytes == 0 );
    }

    if ( !closed || ( failed < requests ) )
    {
        fprintf( stderr, "%s: %d of %d builds failed after %d requests\n", description, failed, int( 2 * requests ), int( requests ) );
    }

    Check( closed && ( failed >= requests ), description );
}

void TestRefusedBlocks()
{
    Shape shape;
    MakeShape( shape );
    CheckRefusals( shape, 4, "refused blocks in a structure" );

    Custom custom;
    custom.kind = Custom::Kind::kLargeObject;
    CheckRefusals( custom, 4, "refused blocks in a large object" );

    custom.kind = Custom::Kind::kStatusesIgnored;
    CheckRefusals( custom, 4, "refused blocks with statuses ignored" );

    Array<double> numbers;

    for ( int a = 0; a < 5000; a++ )
    {
        numbers.AppendArrayElement( a * 0.5 );
    }

    CheckRefusals( numbers, 8, "refused blocks in a long array" );
}

int main()
{
    TestSameAs2x();
    TestRules();
    TestRefusedBlocks();

    return TestResult();
}
