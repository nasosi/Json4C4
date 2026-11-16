#include "TestCheck.h"

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    include <map>
#    include <string>
#    include <vector>

struct Item
{
    double v = 0;

#    define ITEM_PROTO "v", v
    DEFINE_JSON4C4_MEMBER_FUNCTIONS( ITEM_PROTO )
};

struct Outer
{
    Item   inner;
    double w = 0;

#    define OUTER_PROTO "inner", inner, "w", w
    DEFINE_JSON4C4_MEMBER_FUNCTIONS( OUTER_PROTO )
};

struct Aggregate
{
    Terathon::Array<double> a;
};

#    define AGGREGATE_PROTO "a", object.a
DEFINE_JSON4C4_FUNCTIONS( Aggregate, AGGREGATE_PROTO )

struct MoveOnly
{
    Terathon::String<> s;

    MoveOnly() = default;

    MoveOnly( const MoveOnly& )            = delete;
    MoveOnly& operator=( const MoveOnly& ) = delete;

    MoveOnly( MoveOnly&& )            = default;
    MoveOnly& operator=( MoveOnly&& ) = default;

#    define MOVE_ONLY_PROTO "s", s
    DEFINE_JSON4C4_MEMBER_FUNCTIONS( MOVE_ONLY_PROTO )
};

void TestVectorOfMemberStructs()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "[{\"v\":1},{\"v\":2}]" ).status, Json::Status::kOk, "parse an array of objects" );

    std::vector<Item> items;
    CheckStatus( sd.DeserializeTo( items ), Json::Status::kOk, "deserialize a vector of member structs" );
    Check( items.size() == 2 && items[ 0 ].v == 1 && items[ 1 ].v == 2, "vector of member structs values" );

    CheckStatus( Json::Validate( sd, items ), Json::Status::kOk, "validate a vector of member structs" );

    Terathon::Array<Item> itemArray;
    CheckStatus( sd.DeserializeTo( itemArray ), Json::Status::kOk, "deserialize an array of member structs" );
    Check( itemArray.GetArrayElementCount() == 2 && itemArray[ 1 ].v == 2, "array of member structs values" );
}

void TestAggregateWithArray()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "[{\"a\":[1,2]},{\"a\":[]}]" ).status, Json::Status::kOk, "parse aggregates" );

    Terathon::Array<Aggregate> aggregates;
    CheckStatus( Json::Validate( sd, aggregates ), Json::Status::kOk, "validate an array of aggregates with an array member" );
    CheckStatus( sd.DeserializeTo( aggregates ), Json::Status::kOk, "deserialize an array of aggregates with an array member" );
    Check( aggregates.GetArrayElementCount() == 2 && aggregates[ 0 ].a.GetArrayElementCount() == 2, "aggregate values" );

    std::vector<Aggregate> aggregateVector;
    CheckStatus( Json::Validate( sd, aggregateVector ), Json::Status::kOk, "validate a vector of aggregates with an array member" );
}

void TestMoveOnlyElements()
{
    Json::StructuredData sd;
    CheckStatus(
        ParseText( sd, "[{\"s\":\"a\"},{\"s\":\"a string that is longer than the local buffer\"}]" ).status, Json::Status::kOk, "parse move only elements" );

    Terathon::Array<MoveOnly> elements;
    CheckStatus( sd.DeserializeTo( elements ), Json::Status::kOk, "deserialize an array of move only elements" );
    Check( elements.GetArrayElementCount() == 2 && elements[ 1 ].s == "a string that is longer than the local buffer", "move only element values" );
}

void TestNestedMemberStruct()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"inner\":{\"v\":3},\"w\":4}" ).status, Json::Status::kOk, "parse a nested object" );

    Outer outer;
    CheckStatus( Json::Validate( sd, outer ), Json::Status::kOk, "validate a member struct inside a member struct" );
    CheckStatus( sd.DeserializeTo( outer ), Json::Status::kOk, "deserialize a member struct inside a member struct" );
    Check( outer.inner.v == 3 && outer.w == 4, "nested member struct values" );

    outer.inner.v = 5;
    CheckStatus( sd.SerializeFrom( outer ), Json::Status::kOk, "serialize a member struct inside a member struct" );

    Outer roundTrip;
    CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize the serialized nested struct" );
    Check( roundTrip.inner.v == 5 && roundTrip.w == 4, "nested member struct survives the round trip" );

    CheckStatus( ParseText( sd, "{\"inner\":{\"v\":\"text\"},\"w\":4}" ).status, Json::Status::kOk, "parse a nested object with a wrong type" );
    CheckStatus( Json::Validate( sd, outer ), Json::Status::kInvalidValueType, "validate reports a wrong nested type" );
}

void TestDeserializeReplacesMaps()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"b\":2}" ).status, Json::Status::kOk, "parse a map" );

    Json::ObjectMap<double> map;

    auto* element = new Json::ObjectMap<double>::ElementType;
    element->name = "a";
    element->data = 1;
    map.InsertMapElement( element );

    CheckStatus( sd.DeserializeTo( map ), Json::Status::kOk, "deserialize into a non-empty ObjectMap" );
    Check( map.GetMapElementCount() == 1 && map.FindMapElement( "b" ) && !map.FindMapElement( "a" ), "deserialize replaces an ObjectMap" );

    std::map<std::string, double> stdMap = { { "a", 1 } };
    CheckStatus( sd.DeserializeTo( stdMap ), Json::Status::kOk, "deserialize into a non-empty std::map" );
    Check( stdMap.size() == 1 && stdMap.count( "b" ) == 1, "deserialize replaces a std::map" );

    CheckStatus( ParseText( sd, "{\"c\":\"text\"}" ).status, Json::Status::kOk, "parse a map with a wrong type" );
    CheckStatus( sd.DeserializeTo( stdMap ), Json::Status::kInvalidValueType, "deserialize a map with a wrong type" );
    Check( stdMap.size() == 1 && stdMap.count( "b" ) == 1, "a failed deserialize leaves a std::map unchanged" );
}

void TestNestedObjectMaps()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"a\":{\"x\":1,\"y\":2},\"b\":{}}" ).status, Json::Status::kOk, "parse nested objects" );

    Json::ObjectMap<Json::ObjectMap<double>> maps;
    CheckStatus( sd.DeserializeTo( maps ), Json::Status::kOk, "deserialize an ObjectMap of ObjectMaps" );
    Check( maps.FindMapElement( "a" ) && maps.FindMapElement( "a" )->data.FindMapElement( "y" ) &&
               maps.FindMapElement( "a" )->data.FindMapElement( "y" )->data == 2,
           "nested ObjectMap values" );

    CheckStatus( sd.SerializeFrom( maps ), Json::Status::kOk, "serialize an ObjectMap of ObjectMaps" );
    CheckStatus( Json::Validate( sd, maps ), Json::Status::kOk, "validate an ObjectMap of ObjectMaps" );
}

int main()
{
    TestNestedObjectMaps();
    TestVectorOfMemberStructs();
    TestAggregateWithArray();
    TestMoveOnlyElements();
    TestNestedMemberStruct();
    TestDeserializeReplacesMaps();

    return TestResult();
}

#else

int main()
{
    return testSkippedReturnCode;
}

#endif
