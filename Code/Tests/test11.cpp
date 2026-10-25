#include "TestAllocation.h"
#include "TestCheck.h"

struct Repeated
{
    double x = 1;
    double y = 2;
};

#define REPEATED_PROTO "a", object.x, "a", object.y
DEFINE_JSON4C4_FUNCTIONS( Repeated, REPEATED_PROTO )

int CountOccurrences( const char* text, const char* pattern )
{
    int count = 0;

    for ( const char* found = strstr( text, pattern ); found; found = strstr( found + 1, pattern ) )
    {
        count++;
    }

    return count;
}

const char* const duplicateDocument = "{\"a\":1,\"a\":[2,3,4],\"b\":2,\"a\":{\"c\":3}}";

void TestDuplicateKeysRejected()
{
    Json::StructuredData sd;

    Json::ParseResult parseResult = ParseText( sd, duplicateDocument );
    CheckStatus( parseResult.status, Json::Status::kDuplicateName, "duplicate keys are rejected by default" );
    Check( parseResult.errorLine == 1 && parseResult.errorColumn == 8, "the error points at the repeated name" );
    Check( sd.GetRootJsonValue() == nullptr, "a rejected document leaves no root" );

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":{\"a\":2}}" ).status, Json::Status::kOk, "the same name in different objects is not a duplicate" );
}

void TestDuplicateKeysKept( Json::DuplicateNames policy, double expected, const char* description )
{
    Json::ParseOptions options;
    options.duplicateNames = policy;

    Json::StructuredData sd;
    CheckStatus( ParseText( sd, duplicateDocument, options ).status, Json::Status::kOk, description );

    const Json::ObjectValue* root = sd.GetRootJsonValue() ? sd.GetRootJsonValue()->AsJsonObjectValue() : nullptr;
    Check( root && root->GetMapElementCount() == 2, "duplicate keys are stored once" );

    if ( expected != 0 )
    {
        Check( root && root->FindNumber( "a" ) && *root->FindNumber( "a" ) == expected, description );
    }
    else
    {
        Check( root && root->FindJsonObjectValue( "a" ) && root->FindJsonObjectValue( "a" )->FindNumber( "c" ), description );
    }

    CheckStatus( sd.Write( "Data/Test/out_test11_duplicates.json" ), Json::Status::kOk, "write a document that had duplicate keys" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test11_duplicates.json", text ) && CountOccurrences( text, "\"a\"" ) == 1, "a duplicate key is written once" );
    Check( strstr( text, "\"a\"" ) < strstr( text, "\"b\"" ), "a duplicate key keeps the position of its first occurrence" );

#ifdef JSON4C4_TEST_ALLOCATION_HOOK

    long liveBefore = allocationLiveCount;

    for ( int a = 0; a < 100; a++ )
    {
        Json::StructuredData repeated;
        ParseText( repeated, "{\"a\":1,\"a\":[2,3,4],\"a\":{\"b\":[5]}}", options );
    }

    Check( allocationLiveCount == liveBefore, "duplicate keys do not leak" );

#endif
}

void TestOrderSurvivesDirectChanges()
{
    Json::StructuredData sd;
    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":2,\"c\":3}" ).status, Json::Status::kOk, "parse three members" );

    Json::ObjectValue* root = sd.GetRootJsonValue()->AsJsonObjectValue();

    Json::Value* detached = root->FindValue( "c" );

    delete root->FindValue( "a" );
    detached->Detach();

    CheckStatus( sd.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write after deleting and detaching members" );

    Terathon::String<> text;
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n  \"b\" : 2\n}", "deleted and detached members are not written" );

    Json::StructuredData other;
    CheckStatus( ParseText( other, "{\"x\":0}" ).status, Json::Status::kOk, "parse a second object" );

    Json::ObjectValue* otherRoot = other.GetRootJsonValue()->AsJsonObjectValue();
    Check( otherRoot->InsertAccountedMapElement( root->FindValue( "b" ) ), "move a member to another object" );

    CheckStatus( sd.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write the object a member moved out of" );
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n}", "a moved member leaves its old object" );

    CheckStatus( other.Write( "Data/Test/out_test11_direct.json" ), Json::Status::kOk, "write the object a member moved into" );
    Check( ReadTextFile( "Data/Test/out_test11_direct.json", text ) && text == "{\n  \"x\" : 0,\n  \"b\" : 2\n}", "a moved member is written once, last" );

    delete detached;
}

void TestDuplicateNames()
{
    Json::StructuredData sd;

    CheckStatus( sd.SerializeFrom( Repeated() ), Json::Status::kDuplicateName, "a proto that repeats a name" );
    Check( sd.GetRootJsonValue() == nullptr, "a failed SerializeFrom does not set a root" );

#ifdef JSON4C4_TEST_ALLOCATION_HOOK

    long liveBefore = allocationLiveCount;

    {
        Json::StructuredData repeated;
        repeated.SerializeFrom( Repeated() );
    }

    Check( allocationLiveCount == liveBefore, "a repeated name does not leak" );

#endif
}

void TestAccountedMap()
{
    Json::StructuredData sd;

    CheckStatus( ParseText( sd, "{\"a\":1,\"b\":2,\"c\":3}" ).status, Json::Status::kOk, "parse three members" );

    Json::ObjectValue* root = sd.GetRootJsonValue()->AsJsonObjectValue();

    Json::Value* removed = root->RemoveAccountedMapElement( "b" );
    Check( removed != nullptr && removed->name == "b", "remove a member" );
    delete removed;

    Check( root->RemoveAccountedMapElement( "missing" ) == nullptr, "remove a missing member" );

    CheckStatus( sd.Write( "Data/Test/out_test11_accounted.json" ), Json::Status::kOk, "write after removing a member" );

    Json::StructuredData reparsed;
    CheckStatus( reparsed.Parse( "Data/Test/out_test11_accounted.json" ).status, Json::Status::kOk, "reparse after removing a member" );

    const Json::ObjectValue* reparsedRoot = reparsed.GetRootJsonValue()->AsJsonObjectValue();
    Check( reparsedRoot->GetMapElementCount() == 2 && reparsedRoot->FindNumber( "a" ) && reparsedRoot->FindNumber( "c" ) && !reparsedRoot->FindNumber( "b" ),
           "the removed member is not written" );

    Json::Value* element = Json::Detail::CreateValueFor( 5.0 );
    element->name        = "z";
    CheckStatus( Json::Serialize( element, 5.0 ), Json::Status::kOk, "serialize a new member" );
    Check( root->InsertAccountedMapElement( element ), "insert a new member" );

    Json::Value* duplicate = Json::Detail::CreateValueFor( 6.0 );
    duplicate->name        = "z";
    Check( !root->InsertAccountedMapElement( duplicate ), "a duplicate insert is refused" );
    delete duplicate;

    Check( root->FindNumber( "z" ) && *root->FindNumber( "z" ) == 5.0, "the first inserted member stays" );

    root->PurgeAccountedMap();
    Check( root->GetMapElementCount() == 0, "purge removes every member" );

    CheckStatus( sd.Write( "Data/Test/out_test11_accounted.json" ), Json::Status::kOk, "write after purging" );
    CheckStatus( reparsed.Parse( "Data/Test/out_test11_accounted.json" ).status, Json::Status::kOk, "reparse after purging" );
    Check( reparsed.GetRootJsonValue()->AsJsonObjectValue()->GetMapElementCount() == 0, "an empty object is written" );
}

int main()
{
    TestDuplicateKeysRejected();
    TestDuplicateKeysKept( Json::DuplicateNames::kKeepFirst, 1, "the first value of a duplicate key is kept" );
    TestDuplicateKeysKept( Json::DuplicateNames::kKeepLast, 0, "the last value of a duplicate key is kept" );
    TestOrderSurvivesDirectChanges();
    TestDuplicateNames();
    TestAccountedMap();

    return TestResult();
}
