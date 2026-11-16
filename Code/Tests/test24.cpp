// Editing a 3.0 document: members and elements added and removed in place, values replaced, copies between documents,
// objects that grow past 16 members and shrink back, and edits refused or failed leaving the document as it was.

#include "TestCheck.h"

#include <cstdint>
#include <cstdlib>

using Terathon::String;

// Hands out blocks with malloc, refuses the requests from refusedFirst to refusedLast, and checks that every block comes
// back once with its size.
class CountingSource final : public Json::BlockSource
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

    return ( other.ParseText( expected, strlen( expected ) ).status == Json::Status::kOk ) && ( strcmp( WriteNew( data ), WriteNew( other ) ) == 0 );
}

// Whether every member of every object in a value is found by its name, which reads the name index of a large object.
bool FindsEveryMember( const Json::Value& value )
{
    bool found = true;

    for ( const Json::Value& element : value.AsArray() )
    {
        found = found && FindsEveryMember( element );
    }

    for ( const Json::Member& member : value.AsObject() )
    {
        found = found && ( value.AsObject().Find( member.GetName() ) == &member.GetValue() ) && FindsEveryMember( member.GetValue() );
    }

    return found;
}

Json::Status Parse( Json::StructuredData& data, const char* text )
{
    return data.ParseText( text, strlen( text ) ).status;
}

Json::Status AddNumber( Json::StructuredData& data, const Json::Value& object, int number );

void TestMembers()
{
    Json::StructuredData data;

    CheckStatus( Parse( data, "{ \"a\": 1, \"b\": 2, \"c\": 3 }" ), Json::Status::kOk, "parse three members" );

    const Json::Value& root   = *data.GetRoot();
    Json::ObjectView   object = root.AsObject();

    CheckStatus( data.RemoveMember( root, "b" ), Json::Status::kOk, "remove a member" );
    CheckStatus( data.RemoveMember( root, "missing" ), Json::Status::kNameNotPresent, "remove a missing member" );
    CheckStatus( data.AddMember( root, "z", 5.0 ), Json::Status::kOk, "add a member" );
    CheckStatus( data.AddMember( root, "z", 6.0 ), Json::Status::kDuplicateName, "a repeated name is refused" );
    Check( Holds( data, "{ \"a\": 1, \"c\": 3, \"z\": 5 }" ), "members removed and added in order" );
    Check( object.FindNumber( "z" ) && ( *object.FindNumber( "z" ) == 5 ), "a view of the edited object stays valid" );

    Json::ObjectMap<double> map;
    auto*                   element = new Json::ObjectMap<double>::ElementType;

    element->name = "m";
    element->data = 5;
    map.InsertMapElement( element );

    CheckStatus( data.SerializeAt( root, map ), Json::Status::kOk, "serialize a map onto an object" );
    Check( Holds( data, "{ \"m\": 5 }" ), "only the map's members are left" );

    CheckStatus( data.SerializeAt( root, Json::ObjectMap<double>() ), Json::Status::kOk, "serialize an empty map onto an object" );
    Check( Holds( data, "{}" ), "an empty map empties the object" );

    for ( int a = 0; a < 20; a++ )
    {
        AddNumber( data, root, a );
    }

    CheckStatus( data.EmptyObject( root ), Json::Status::kOk, "empty a large object" );
    CheckStatus( AddNumber( data, root, 7 ), Json::Status::kOk, "add to the emptied object" );
    Check( Holds( data, "{ \"k7\": 7 }" ) && FindsEveryMember( root ), "an emptied object takes new members" );
    CheckStatus( data.EmptyObject( *root.AsObject().Find( "k7" ) ), Json::Status::kInvalidValueType, "empty a number" );

    CheckStatus( data.AppendElement( root, 1.0 ), Json::Status::kInvalidValueType, "append to an object" );
    CheckStatus( data.RemoveElement( root, 0 ), Json::Status::kInvalidValueType, "remove an element of an object" );
}

// Empty arrays and objects hold no room until the first addition.
void TestEmptyContainers()
{
    Json::StructuredData data;

    CheckStatus( Parse( data, "{ \"a\": [], \"o\": {} }" ), Json::Status::kOk, "parse an empty array and object" );

    const Json::Value& root = *data.GetRoot();

    CheckStatus( data.AppendElement( *root.AsObject().Find( "a" ), true ), Json::Status::kOk, "append to an empty array" );
    CheckStatus( data.AddMember( *root.AsObject().Find( "o" ), "k", String<>( "v" ) ), Json::Status::kOk, "add to an empty object" );
    Check( Holds( data, "{ \"a\": [ true ], \"o\": { \"k\": \"v\" } }" ) && FindsEveryMember( root ), "the first element and member" );
    CheckStatus( data.RemoveMember( *root.AsObject().Find( "o" ), "k" ), Json::Status::kOk, "remove the only member" );
    CheckStatus( data.RemoveElement( *root.AsObject().Find( "a" ), 0 ), Json::Status::kOk, "remove the only element" );
    Check( Holds( data, "{ \"a\": [], \"o\": {} }" ), "empty again" );
}

void TestElements()
{
    Json::StructuredData data;

    CheckStatus( Parse( data, "[ 1, 2, 3, 4 ]" ), Json::Status::kOk, "parse an array" );

    const Json::Value& root  = *data.GetRoot();
    Json::ArrayView    array = root.AsArray();
    bool               same  = true;

    for ( int a = 0; a < 40; a++ )
    {
        same = same && ( data.AppendElement( root, double( a ) ) == Json::Status::kOk );
    }

    Check( same && ( array.GetElementCount() == 44 ), "append forty elements" );

    for ( int a = 0; same && ( a < 44 ); a++ )
    {
        same = array[ a ].AsNumber() && ( *array[ a ].AsNumber() == ( ( a < 4 ) ? a + 1 : a - 4 ) );
    }

    Check( same, "the elements are kept in order" );

    CheckStatus( data.RemoveElement( root, 0 ), Json::Status::kOk, "remove the first element" );
    CheckStatus( data.RemoveElement( root, 43 ), Json::Status::kIndexOutOfRange, "remove past the end" );
    Check( ( array.GetElementCount() == 43 ) && ( *array[ 0 ].AsNumber() == 2 ) && ( *array[ 42 ].AsNumber() == 39 ), "the rest move down" );

    CheckStatus( data.SerializeAt( array[ 1 ], String<>( "three" ) ), Json::Status::kOk, "replace an element" );
    Check( array[ 1 ].AsString() == "three", "the replaced element" );

    CheckStatus( data.AddMember( root, "a", 1.0 ), Json::Status::kInvalidValueType, "add a member to an array" );
}

// The members k<n> of an object for the numbers given, in order. Each is found by name and the order is kept.
bool HasMembers( const Json::Value& object, const int* numbers, int count )
{
    Json::ObjectView view = object.AsObject();
    bool             same = view.GetMemberCount() == uint32_t( count );

    for ( int a = 0; same && ( a < count ); a++ )
    {
        char name[ 16 ];
        snprintf( name, sizeof( name ), "k%d", numbers[ a ] );

        same = ( view.begin()[ a ].GetName() == name ) && view.FindNumber( name ) && ( *view.FindNumber( name ) == numbers[ a ] );
    }

    return same && !view.Find( "k-1" );
}

Json::Status AddNumber( Json::StructuredData& data, const Json::Value& object, int number )
{
    char name[ 16 ];
    snprintf( name, sizeof( name ), "k%d", number );

    return data.AddMember( object, name, double( number ) );
}

Json::Status RemoveNumber( Json::StructuredData& data, const Json::Value& object, int number )
{
    char name[ 16 ];
    snprintf( name, sizeof( name ), "k%d", number );

    return data.RemoveMember( object, name );
}

// A parsed object of 20 members, whose index follows its last member, first grows or first shrinks below 16 members,
// then grows to 40, shrinks and grows again, and every member is found after each edit.
bool EditAcrossSixteen( bool growFirst )
{
    String<> text = "{";

    for ( int a = 0; a < 20; a++ )
    {
        char member[ 32 ];
        snprintf( member, sizeof( member ), "%s\"k%d\":%d", ( a != 0 ) ? "," : "", a, a );
        text += member;
    }

    text += "}";

    Json::StructuredData data;

    if ( Parse( data, text ) != Json::Status::kOk )
    {
        return false;
    }

    const Json::Value& object = *data.GetRoot();
    int                numbers[ 64 ];
    int                count    = 20;
    int                next     = 100;
    int                removals = 0;
    bool               same     = true;

    for ( int a = 0; a < count; a++ )
    {
        numbers[ a ] = a;
    }

    auto remove = [ & ]( int target )
    {
        while ( same && ( count > target ) )
        {
            int at = ( removals++ * 7 ) % count;

            same = RemoveNumber( data, object, numbers[ at ] ) == Json::Status::kOk;

            for ( int a = at + 1; a < count; a++ )
            {
                numbers[ a - 1 ] = numbers[ a ];
            }

            count--;
            same = same && HasMembers( object, numbers, count );
        }
    };

    auto add = [ & ]( int target )
    {
        while ( same && ( count < target ) )
        {
            same               = AddNumber( data, object, next ) == Json::Status::kOk;
            numbers[ count++ ] = next++;
            same               = same && HasMembers( object, numbers, count );
        }
    };

    if ( growFirst )
    {
        add( 22 );
    }

    remove( 5 );
    add( 40 );
    remove( 10 );
    add( 30 );

    return same;
}

void TestCrossingSixteen()
{
    Check( EditAcrossSixteen( false ), "members are found across 16 in both directions" );
    Check( EditAcrossSixteen( true ), "members are found after a large parsed object grows" );
}

// A value copied into the array or object it comes from is built before the array or object changes.
void TestSelfCopies()
{
    Json::StructuredData data;

    CheckStatus( Parse( data, "{ \"list\": [ 1, [ 2 ] ], \"x\": { \"y\": \"z\" } }" ), Json::Status::kOk, "parse a list and an object" );

    const Json::Value& root = *data.GetRoot();

    CheckStatus( data.AppendElement( *root.AsObject().Find( "list" ), *root.AsObject().Find( "list" ) ), Json::Status::kOk, "append an array to itself" );
    CheckStatus( data.AddMember( root, "copy", *root.AsObject().Find( "x" ) ), Json::Status::kOk, "add a copy of a member to its object" );
    Check( Holds( data, "{ \"list\": [ 1, [ 2 ], [ 1, [ 2 ] ] ], \"x\": { \"y\": \"z\" }, \"copy\": { \"y\": \"z\" } }" ) && FindsEveryMember( root ),
           "the copies" );
}

void TestOtherDocuments()
{
    Json::StructuredData from;
    Json::StructuredData to;

    CheckStatus( Parse( from, "{ \"a\": 1, \"b\": { \"c\": [ true, null, \"s\" ] } }" ), Json::Status::kOk, "parse the first document" );
    CheckStatus( Parse( to, "{ \"x\": 0 }" ), Json::Status::kOk, "parse the second document" );

    CheckStatus( to.AddMember( *to.GetRoot(), "b", *from.GetRoot()->AsObject().Find( "b" ) ), Json::Status::kOk, "copy a member to another document" );
    CheckStatus( from.RemoveMember( *from.GetRoot(), "b" ), Json::Status::kOk, "remove it from the first" );
    Check( Holds( from, "{ \"a\": 1 }" ) && Holds( to, "{ \"x\": 0, \"b\": { \"c\": [ true, null, \"s\" ] } }" ) && FindsEveryMember( *from.GetRoot() ) &&
               FindsEveryMember( *to.GetRoot() ),
           "a member moved between documents" );

    CheckStatus( to.RemoveMember( *from.GetRoot(), "a" ), Json::Status::kInvalidValueType, "a value of another document is refused" );
    CheckStatus( to.SerializeAt( *from.GetRoot(), 1.0 ), Json::Status::kInvalidValueType, "replacing a value of another document is refused" );
    Check( Holds( from, "{ \"a\": 1 }" ), "the other document is left alone" );
}

// A Serialize that tries to edit the document it is building.
struct Meddler
{
    Json::StructuredData* data   = nullptr;
    const Json::Value*    target = nullptr;
    mutable Json::Status  edit   = Json::Status::kOk;
    mutable Json::Status  build  = Json::Status::kOk;
};

Json::Status Serialize( Json::Node node, const Meddler& meddler ) noexcept
{
    meddler.edit  = meddler.data->AppendElement( *meddler.target, 1.0 );
    meddler.build = meddler.data->SerializeFrom( 2.0 );

    return node.SetNumber( 3 );
}

void TestEditsDuringBuild()
{
    Json::StructuredData data;
    Meddler              meddler;

    CheckStatus( Parse( data, "[ 1 ]" ), Json::Status::kOk, "parse an array" );

    meddler.data   = &data;
    meddler.target = data.GetRoot();

    CheckStatus( data.AppendElement( *data.GetRoot(), meddler ), Json::Status::kOk, "append through a Serialize that edits" );
    CheckStatus( meddler.edit, Json::Status::kInvalidValueType, "an edit during a build is refused" );
    CheckStatus( meddler.build, Json::Status::kInvalidValueType, "a SerializeFrom during a build is refused" );
    Check( Holds( data, "[ 1, 3 ]" ), "the build goes on" );
}

// Parses an object and edits it: members added past 16, forty elements appended to an array in it, the parsed members
// removed.
// Each edit either succeeds or is refused and leaves the written text as it was.
bool RunEdits( CountingSource& source, int& refusedEdits )
{
    Json::StructuredData data( source );

    const char*  text   = "{ \"array\": [ 1, 2, 3, 4 ], \"k0\": 0, \"k1\": 1, \"k2\": 2, \"k3\": 3, \"k4\": 4, \"k5\": 5, \"k6\": 6, \"k7\": 7 }";
    Json::Status status = Parse( data, text );

    if ( status != Json::Status::kOk )
    {
        return status == Json::Status::kAllocationRefused;
    }

    const Json::Value& root = *data.GetRoot();
    bool               same = true;

    for ( int step = 0; same && ( step < 68 ); step++ )
    {
        String<> before = WriteNew( data );

        if ( step < 20 )
        {
            status = AddNumber( data, root, 8 + step );
        }
        else if ( step < 60 )
        {
            status = data.AppendElement( *root.AsObject().Find( "array" ), double( step ) );
        }
        else
        {
            status = RemoveNumber( data, root, step - 60 );
        }

        if ( status == Json::Status::kAllocationRefused )
        {
            same = strcmp( WriteNew( data ), before ) == 0;
            refusedEdits++;
        }
        else
        {
            same = status == Json::Status::kOk;
        }

        same = same && FindsEveryMember( root );
    }

    return same;
}

// Refuses each block request of the edits in turn, and then every request from it on.
void TestRefusedEdits()
{
    CountingSource source;
    int            refusedEdits = 0;

    Check( RunEdits( source, refusedEdits ) && ( refusedEdits == 0 ), "edits through a counting source" );

    int64_t requests = source.requestCount;
    bool    closed   = ( requests >= 6 ) && ( source.outstandingCount == 0 );

    for ( int64_t refused = 0; closed && ( refused < 2 * requests ); refused++ )
    {
        source.requestCount = 0;
        source.refusedFirst = refused % requests;
        source.refusedLast  = ( refused < requests ) ? source.refusedFirst : INT64_MAX;

        closed = RunEdits( source, refusedEdits ) && ( source.outstandingCount == 0 ) && ( source.outstandingBytes == 0 );
    }

    if ( !closed || ( refusedEdits == 0 ) )
    {
        fprintf( stderr, "refused edits: %d after %d requests\n", refusedEdits, int( requests ) );
    }

    Check( closed && ( refusedEdits > 0 ), "a refused edit leaves the document as it was" );
}

int main()
{
    TestMembers();
    TestElements();
    TestEmptyContainers();
    TestCrossingSixteen();
    TestSelfCopies();
    TestOtherDocuments();
    TestEditsDuringBuild();
    TestRefusedEdits();

    return TestResult();
}
