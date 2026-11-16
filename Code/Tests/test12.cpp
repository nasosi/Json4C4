#include "TestAllocation.h"
#include "TestCheck.h"

namespace V3 = Json::V3;

struct Point
{
    double x = 0;
    double y = 0;
};

#define POINT_PROTO "x", object.x, "y", object.y
DEFINE_JSON4C4_FUNCTIONS( Point, POINT_PROTO )

struct Repeated
{
    double x = 1;
};

#define REPEATED_PROTO "a", object.x, "a", object.x
DEFINE_JSON4C4_FUNCTIONS( Repeated, REPEATED_PROTO )

void TestNullRoot()
{
    V3::StructuredData sd;
    Point              point;

    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo without a root" );
    CheckStatus( V3::Validate( sd, point ), Json::Status::kInvalidStructuredData, "Validate without a root" );

    CheckStatus( ParseText( sd, "{\"x\":" ).status, Json::Status::kExpectedJsonValue, "a failed parse" );
    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo after a failed parse" );
    CheckStatus( V3::Validate( sd, point ), Json::Status::kInvalidStructuredData, "Validate after a failed parse" );

    V3::Value nullValue;
    V3::Node  nodeNeverGiven;

    double value = 0;
    CheckStatus( V3::Deserialize( nullValue, "x", value ), Json::Status::kInvalidValueType, "Deserialize from a null value" );
    CheckStatus( V3::Validate( nullValue, "x", value ), Json::Status::kInvalidValueType, "Validate a null value" );
    CheckStatus( sd.AddMember( nullValue, "x", value ), Json::Status::kInvalidValueType, "add a member to a null value" );

    Terathon::Array<double> array;
    CheckStatus( V3::Deserialize( nullValue, array ), Json::Status::kInvalidValueType, "Deserialize an array from a null value" );
    CheckStatus( V3::Validate( nullValue, array ), Json::Status::kInvalidValueType, "Validate an array against a null value" );
    CheckStatus( V3::Serialize( nodeNeverGiven, array ), Json::Status::kInvalidValueType, "Serialize an array into a node that was never given" );

    Json::ObjectMap<double> map;
    CheckStatus( V3::Deserialize( nullValue, map ), Json::Status::kInvalidValueType, "Deserialize a map from a null value" );
    CheckStatus( V3::Validate( nullValue, map ), Json::Status::kInvalidValueType, "Validate a map against a null value" );
    CheckStatus( V3::Serialize( nodeNeverGiven, map ), Json::Status::kInvalidValueType, "Serialize a map into a node that was never given" );
}

void TestFailedParseClearsDocument()
{
    V3::StructuredData sd;
    Point              point;

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2} x" ).status, Json::Status::kExpectedEndOfFile, "trailing text" );
    Check( sd.GetRoot() == nullptr, "trailing text leaves no root" );
    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo after trailing text" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document" );
    CheckStatus( sd.Parse( "Data/Test/missing_file.json" ).status, Json::Status::kFileOpenError, "parse a missing file" );
    Check( sd.GetRoot() == nullptr, "a missing file clears the old document" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document again" );
    CheckStatus( ParseText( sd, "" ).status, Json::Status::kTextBufferContentsInvalid, "parse an empty buffer" );
    Check( sd.GetRoot() == nullptr, "an empty buffer clears the old document" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document once more" );

    Terathon::Array<char> unterminated;
    unterminated.AppendArrayElement( '1' );
    unterminated.AppendArrayElement( '2' );
    CheckStatus( sd.Parse( unterminated ).status, Json::Status::kUnterminatedTextBuffer, "parse an unterminated buffer" );
    Check( sd.GetRoot() == nullptr, "an unterminated buffer clears the old document" );
}

// A failed SerializeFrom leaves the document empty, and a failed SerializeAt leaves it as it was.
void TestFailedSerialize()
{
    V3::StructuredData sd;

    Point point;
    point.x = 3;
    point.y = 4;

    CheckStatus( sd.SerializeFrom( point ), Json::Status::kOk, "serialize a point" );
    CheckStatus( sd.SerializeAt( *sd.GetRoot(), Repeated() ), Json::Status::kDuplicateName, "a failing SerializeAt" );

    Point roundTrip;
    CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "the old document survives a failed SerializeAt" );
    Check( roundTrip.x == 3 && roundTrip.y == 4, "the old document's values survive" );

    CheckStatus( sd.SerializeFrom( Repeated() ), Json::Status::kDuplicateName, "a failing SerializeFrom" );
    Check( sd.GetRoot() == nullptr, "a failed SerializeFrom leaves the document empty" );
}

void TestSerializeMapOntoObject()
{
    Json::ObjectMap<double> map;

    auto* element = new Json::ObjectMap<double>::ElementType;
    element->name = "m";
    element->data = 5;
    map.InsertMapElement( element );

#ifdef JSON4C4_TEST_ALLOCATION_HOOK
    long liveBefore = allocationLiveCount;
#endif

    {
        V3::StructuredData sd;

        CheckStatus( ParseText( sd, "{\"q\":[1,2,3],\"r\":{\"s\":1}}" ).status, Json::Status::kOk, "parse an object" );
        CheckStatus( sd.SerializeAt( *sd.GetRoot(), map ), Json::Status::kOk, "serialize a map onto an object with members" );
        CheckStatus( sd.Write( "Data/Test/out_test12_map.json" ), Json::Status::kOk, "write the replaced object" );

        V3::StructuredData reparsed;
        CheckStatus( reparsed.Parse( "Data/Test/out_test12_map.json" ).status, Json::Status::kOk, "reparse the replaced object" );

        V3::ObjectView root = reparsed.GetRoot()->AsObject();
        Check( root.GetMemberCount() == 1 && root.FindNumber( "m" ) && *root.FindNumber( "m" ) == 5, "only the map's members are written" );
    }

#ifdef JSON4C4_TEST_ALLOCATION_HOOK
    Check( allocationLiveCount == liveBefore, "serializing a map onto an object does not leak" );
#endif
}

int main()
{
    TestNullRoot();
    TestFailedParseClearsDocument();
    TestFailedSerialize();
    TestSerializeMapOntoObject();

    return TestResult();
}
