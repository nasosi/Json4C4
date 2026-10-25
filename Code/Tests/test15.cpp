#include "TestCheck.h"

struct Shape
{
    Terathon::String<> text;
    double             width  = 0;
    double             height = 0;
    double             depth  = -1;
};

#define SHAPE_PROTO "text", object.text, "width", object.width, "height", object.height, Json::optional, "depth", object.depth
DEFINE_JSON4C4_FUNCTIONS( Shape, SHAPE_PROTO )

const char* const roundTripFiles[] = { "Data/Examples/car.json",
                                       "Data/Examples/simple.json",
                                       "Data/Examples/simple_with_array.json",
                                       "Data/Test/test_file01.json",
                                       "Data/Test/test_file02.json",
                                       "Data/Test/test_file04.json",
                                       "Data/Test/jsonchecker/pass1.json",
                                       "Data/Test/jsonchecker/pass2.json",
                                       "Data/Test/jsonchecker/pass3.json",
                                       "Data/Test/jsonchecker/fail1_PASSES_RFC_8259.json",
                                       "Data/Test/jsonchecker/fail18_PASSES_RFC_8259.json" };

void TestWriterIsStable()
{
    for ( const char* fileName : roundTripFiles )
    {
        Json::StructuredData sd;
        CheckStatus( sd.Parse( fileName ).status, Json::Status::kOk, fileName );
        CheckStatus( sd.Write( "Data/Test/out_test15_first.json" ), Json::Status::kOk, "first write" );

        Json::StructuredData reparsed;
        CheckStatus( reparsed.Parse( "Data/Test/out_test15_first.json" ).status, Json::Status::kOk, "reparse the first write" );
        CheckStatus( reparsed.Write( "Data/Test/out_test15_second.json" ), Json::Status::kOk, "second write" );

        Terathon::String<> first;
        Terathon::String<> second;
        Check( ReadTextFile( "Data/Test/out_test15_first.json", first ) && ReadTextFile( "Data/Test/out_test15_second.json", second ) && first == second,
               "writing a reparsed document gives the same text" );
    }
}

void TestInsertionOrderAndIndentation()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"z\":1,\"a\":[true,null],\"m\":{}}" ).status, Json::Status::kOk, "parse members in a non-sorted order" );

    Terathon::String<> text;

    CheckStatus( sd.Write( "Data/Test/out_test15_indent.json" ), Json::Status::kOk, "write with the default indentation" );
    Check( ReadTextFile( "Data/Test/out_test15_indent.json", text ) && text == "{\n  \"z\" : 1,\n  \"a\" : [\n    true,\n    null\n  ],\n  \"m\" : {\n  }\n}",
           "default indentation and insertion order" );

    CheckStatus( sd.Write( "Data/Test/out_test15_indent.json", 1, '\t' ), Json::Status::kOk, "write with tabs" );
    Check( ReadTextFile( "Data/Test/out_test15_indent.json", text ) && text == "{\n\t\"z\" : 1,\n\t\"a\" : [\n\t\ttrue,\n\t\tnull\n\t],\n\t\"m\" : {\n\t}\n}",
           "tab indentation" );

    CheckStatus( sd.Write( "Data/Test/out_test15_indent.json", 0 ), Json::Status::kOk, "write without indentation" );
    Check( ReadTextFile( "Data/Test/out_test15_indent.json", text ) && text == "{\n\"z\" : 1,\n\"a\" : [\ntrue,\nnull\n],\n\"m\" : {\n}\n}", "no indentation" );
}

void TestFindAccessors()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"s\":\"text\",\"n\":2,\"b\":false,\"z\":null,\"a\":[1],\"o\":{\"x\":1}}" ).status, Json::Status::kOk, "parse every type" );

    const Json::ObjectValue* root = sd.GetRootJsonValue()->AsJsonObjectValue();

    Check( root->FindString( "s" ) && *root->FindString( "s" ) == "text", "FindString" );
    Check( root->FindNumber( "n" ) && *root->FindNumber( "n" ) == 2, "FindNumber" );
    Check( root->FindBool( "b" ) && *root->FindBool( "b" ) == false, "FindBool" );
    Check( root->FindJsonNull( "z" ) != nullptr, "FindJsonNull" );
    Check( root->FindJsonValueArray( "a" ) && root->FindJsonValueArray( "a" )->GetArrayElementCount() == 1, "FindJsonValueArray" );
    Check( root->FindJsonObjectValue( "o" ) && root->FindJsonObjectValue( "o" )->FindNumber( "x" ), "FindJsonObjectValue" );

    Check( !root->FindString( "n" ) && !root->FindNumber( "s" ) && !root->FindBool( "z" ) && !root->FindJsonNull( "b" ), "Find with the wrong type" );
    Check( !root->FindJsonValueArray( "o" ) && !root->FindJsonObjectValue( "a" ), "Find containers with the wrong type" );
    Check( !root->FindString( "missing" ) && !root->FindJsonObjectValue( "missing" ), "Find a missing name" );

    int count = 0;

    for ( const Json::Value* value : *root )
    {
        Check( value->name.GetStringLength() == 1, "iterate over the members" );
        count++;
    }

    Check( count == 6, "iteration visits every member" );
}

void TestProtoFields()
{
    Json::StructuredData sd;
    Shape                shape;

    CheckStatus( ParseText( sd, "{\"text\":\"Color\",\"width\":5,\"height\":10}" ).status, Json::Status::kOk, "parse a shape without the optional field" );
    CheckStatus( Json::Validate( sd, shape ), Json::Status::kOk, "validate a shape without the optional field" );
    CheckStatus( sd.DeserializeTo( shape ), Json::Status::kOk, "deserialize a shape without the optional field" );
    Check( shape.text == "Color" && shape.width == 5 && shape.height == 10 && shape.depth == -1, "shape values" );

    CheckStatus(
        ParseText( sd, "{\"text\":\"Color\",\"width\":5,\"height\":10,\"depth\":3}" ).status, Json::Status::kOk, "parse a shape with the optional field" );
    CheckStatus( sd.DeserializeTo( shape ), Json::Status::kOk, "deserialize a shape with the optional field" );
    Check( shape.depth == 3, "optional field value" );

    CheckStatus( ParseText( sd, "{\"text\":\"Color\",\"width\":5}" ).status, Json::Status::kOk, "parse a shape without a required field" );
    CheckStatus( Json::Validate( sd, shape ), Json::Status::kNameNotPresent, "validate a shape without a required field" );
    CheckStatus( sd.DeserializeTo( shape ), Json::Status::kNameNotPresent, "deserialize a shape without a required field" );

    CheckStatus( ParseText( sd, "{\"text\":\"Color\",\"width\":\"5\",\"height\":10}" ).status, Json::Status::kOk, "parse a shape with a wrong type" );
    CheckStatus( Json::Validate( sd, shape ), Json::Status::kInvalidValueType, "validate a shape with a wrong type" );
    CheckStatus( sd.DeserializeTo( shape ), Json::Status::kInvalidValueType, "deserialize a shape with a wrong type" );

    CheckStatus( ParseText( sd, "{\"text\":\"Color\",\"width\":5,\"height\":10,\"depth\":true}" ).status,
                 Json::Status::kOk,
                 "parse a shape with a wrong optional type" );
    CheckStatus( Json::Validate( sd, shape ), Json::Status::kInvalidValueType, "validate a shape with a wrong optional type" );

    CheckStatus( ParseText( sd, "[1]" ).status, Json::Status::kOk, "parse an array root" );
    CheckStatus( sd.DeserializeTo( shape ), Json::Status::kInvalidValueType, "deserialize a struct from an array" );

    shape.depth = 7;
    CheckStatus( sd.SerializeFrom( shape ), Json::Status::kOk, "serialize a shape" );

    Shape roundTrip;
    CheckStatus( sd.DeserializeTo( roundTrip ), Json::Status::kOk, "deserialize the serialized shape" );
    Check( roundTrip.text == shape.text && roundTrip.width == shape.width && roundTrip.height == shape.height && roundTrip.depth == 7, "shape round trip" );
}

int main()
{
    Check( strcmp( Json::GetVersion(), "" ) != 0, "version string" );

    TestWriterIsStable();
    TestInsertionOrderAndIndentation();
    TestFindAccessors();
    TestProtoFields();

    return TestResult();
}
