#include "TestAllocation.h"
#include "TestCheck.h"

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
    Json::StructuredData sd;
    Point                point;

    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo without a root" );
    CheckStatus( Json::Validate( sd, point ), Json::Status::kInvalidStructuredData, "Validate without a root" );

    CheckStatus( ParseText( sd, "{\"x\":" ).status, Json::Status::kExpectedJsonValue, "a failed parse" );
    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo after a failed parse" );
    CheckStatus( Json::Validate( sd, point ), Json::Status::kInvalidStructuredData, "Validate after a failed parse" );

    double value = 0;
    CheckStatus( Json::Deserialize( nullptr, "x", value ), Json::Status::kInvalidValueType, "Deserialize from a null value" );
    CheckStatus( Json::Validate( nullptr, "x", value ), Json::Status::kInvalidValueType, "Validate a null value" );
    CheckStatus( Json::Serialize( nullptr, "x", value ), Json::Status::kInvalidValueType, "Serialize into a null value" );

    Terathon::Array<double> array;
    CheckStatus( Json::Deserialize( nullptr, array ), Json::Status::kInvalidValueType, "Deserialize an array from a null value" );
    CheckStatus( Json::Validate( nullptr, array ), Json::Status::kInvalidValueType, "Validate an array against a null value" );
    CheckStatus( Json::Serialize( nullptr, array ), Json::Status::kInvalidValueType, "Serialize an array into a null value" );

    Json::ObjectMap<double> map;
    CheckStatus( Json::Deserialize( nullptr, map ), Json::Status::kInvalidValueType, "Deserialize a map from a null value" );
    CheckStatus( Json::Validate( nullptr, map ), Json::Status::kInvalidValueType, "Validate a map against a null value" );
    CheckStatus( Json::Serialize( nullptr, map ), Json::Status::kInvalidValueType, "Serialize a map into a null value" );
}

void TestFailedParseClearsDocument()
{
    Json::StructuredData sd;
    Point                point;

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2} x" ).status, Json::Status::kExpectedEndOfFile, "trailing text" );
    Check( sd.GetRootJsonValue() == nullptr, "trailing text leaves no root" );
    CheckStatus( sd.DeserializeTo( point ), Json::Status::kInvalidStructuredData, "DeserializeTo after trailing text" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document" );
    CheckStatus( sd.Parse( "Data/Test/missing_file.json" ).status, Json::Status::kFileOpenError, "parse a missing file" );
    Check( sd.GetRootJsonValue() == nullptr, "a missing file clears the old document" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document again" );
    CheckStatus( ParseText( sd, "" ).status, Json::Status::kTextBufferContentsInvalid, "parse an empty buffer" );
    Check( sd.GetRootJsonValue() == nullptr, "an empty buffer clears the old document" );

    CheckStatus( ParseText( sd, "{\"x\":1,\"y\":2}" ).status, Json::Status::kOk, "parse a document once more" );

    Terathon::Array<char> unterminated;
    unterminated.AppendArrayElement( '1' );
    unterminated.AppendArrayElement( '2' );
    CheckStatus( sd.Parse( unterminated ).status, Json::Status::kUnterminatedTextBuffer, "parse an unterminated buffer" );
    Check( sd.GetRootJsonValue() == nullptr, "an unterminated buffer clears the old document" );
}

void TestFailedSerializeKeepsDocument()
{
    Json::StructuredData sd;

    Point point;
    point.x = 3;
    point.y = 4;

    CheckStatus( sd.SerializeFrom( point ), Json::Status::kOk, "serialize a point" );
    CheckStatus( sd.SerializeFrom( Repeated() ), Json::Status::kDuplicateName, "a failing SerializeFrom" );

    Point roundTrip;
    CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "the old document survives a failed SerializeFrom" );
    Check( roundTrip.x == 3 && roundTrip.y == 4, "the old document's values survive" );
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
        Json::StructuredData sd;

        CheckStatus( ParseText( sd, "{\"q\":[1,2,3],\"r\":{\"s\":1}}" ).status, Json::Status::kOk, "parse an object" );
        CheckStatus( Json::Serialize( sd.GetRootJsonValue(), map ), Json::Status::kOk, "serialize a map onto an object with members" );
        CheckStatus( sd.Write( "Data/Test/out_test12_map.json" ), Json::Status::kOk, "write the replaced object" );

        Json::StructuredData reparsed;
        CheckStatus( reparsed.Parse( "Data/Test/out_test12_map.json" ).status, Json::Status::kOk, "reparse the replaced object" );

        const Json::ObjectValue* root = reparsed.GetRootJsonValue()->AsJsonObjectValue();
        Check( root->GetMapElementCount() == 1 && root->FindNumber( "m" ) && *root->FindNumber( "m" ) == 5, "only the map's members are written" );
    }

#ifdef JSON4C4_TEST_ALLOCATION_HOOK
    Check( allocationLiveCount == liveBefore, "serializing a map onto an object does not leak" );
#endif
}

int main()
{
    TestNullRoot();
    TestFailedParseClearsDocument();
    TestFailedSerializeKeepsDocument();
    TestSerializeMapOntoObject();

    return TestResult();
}
