<!--
  Title: Json4C4
  Description: general-purpose JSON library developed to integrate well with the C4 Engine
  Author: Nasos
-->

# Json4C4
Json4C4 is a general-purpose JSON library developed to integrate well with the [C4 Engine](https://c4engine.com/). Json4C4 can also be used in any C++ application if desired.

Json4C4 is appropriate for parsing, deserialization, serialization, validation and editing. The library provides ways for the simple definition of the data structure prototype to enable seamless interoperability with user-defined objects. It is compliant with the [RFC 8259 (STD90) standard](https://datatracker.ietf.org/doc/html/rfc8259) and supports accurate JSON file syntax-error reporting. It is fast, and its number conversions are exact and identical in every build.

Json4C4  has no external dependencies. It internally uses parts of the [Terathon Container](https://github.com/EricLengyel/Terathon-Container-Library), the [OpenDDL Reference Implementation](https://github.com/EricLengyel/OpenDDL) and  [Terathon Math](https://github.com/EricLengyel/Terathon-Math-Library) libraries, and the [NumberTextThree](https://github.com/nasosi/NumberTextThree) header for number conversions.

## Simple example
The use of Json4C4 includes three main steps. Assume for example a ```struct``` like the following:
```cxx
struct TextBox
{
    Terathon::String<> text;
    double             width  = 0;
    double             height = 0;
};
```
First, you need to define the process rules for deserialization/serialization/validation:
```cxx
Json::Status DeserializeTraced( const Json::Value& value, TextBox& object, Json::ReadPath& path )
{
    return Json::DeserializeProto( value, path,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
    );
}

Json::Status Serialize( Json::Node node, const TextBox& object )
{
    return Json::SerializeProto( node,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
    );
}

Json::Status ValidateTraced( const Json::Value& value, const TextBox& object, Json::ReadPath& path )
{
    return Json::ValidateProto( value, path,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
    );
}
```
The ```path``` collects the names and array indices that lead to a value that fails, so that ```Json::Read``` can say where it is (see below).

Alternatively, you can use Json4C4's ```DEFINE_JSON4C4_FUNCTIONS``` macro, which generates all three definitions:
```cxx
#define TEXTBOX_PROTO \
    "text",   object.text,\
    "width",  object.width,\
    "height", object.height

DEFINE_JSON4C4_FUNCTIONS(TextBox, TEXTBOX_PROTO)
```
Second, you create a ```Json::StructuredData``` and parse the file:
```cxx
Json::StructuredData jsonStructuredData;

auto parseResult = jsonStructuredData.Parse( "Data/Examples/simple.json" );
```
And last, you deserialize the structure. If this call is successful, the data of the file has been assigned to the ```textBox``` object member variables:
```cxx
if ( jsonStructuredData.DeserializeTo( textBox ) != Json::Status::kOk )
{
    fprintf( stderr, "Deserialization error\n" );
    return 1;
}

printf( "TextBox text: %s. Width: %f, height: %f\n",
        static_cast< const char* >( textBox.text ), textBox.width, textBox.height );
```
If the ```DeserializeTo``` call is not successful, the ```textBox``` object is in an undefined state. If, in your code, an undefined state is not handled, then you likely need to validate the structured data before deserializing. ```Example02_non_trivial_data_structure``` shows how to achieve this, along with demonstrating additional features of the Json4C4 library.

### Reading in one call
```Json::Read``` parses a file, or a text in memory, and deserializes it in one call. When it fails, the ```Json::ReadResult``` it returns tells where: a syntax error gives its line and column, and a value that does not fit the structure gives its path, such as ```boxes[2].width```, and its line and column. A member that is missing gives the line and column of the object that should hold it.
```cxx
TextBox          textBox;
Json::ReadResult result = Json::Read( "Data/Examples/simple.json", textBox );

if ( result.status != Json::Status::kOk )
{
    fprintf( stderr, "%s at %s, line %d, column %d\n",
             static_cast< const char* >( Json::StatusToString( result.status ) ),
             static_cast< const char* >( result.path ), int( result.errorLine ), int( result.errorColumn ) );
}
```

### Optional arguments
For optional arguments, you can use the keyword ```Json::optional``` to designate them as such before the field name. For example, in the following, absence of ```height``` in the JSON file will not result in an error and the relevant member will be ignored:
```cxx
#define TEXTBOX_PROTO \
    "text",   object.text,\
    "width",  object.width,\
    Json::optional, "height", object.height
```
An optional member that is present must be valid as a whole: a member missing inside it fails like any other.

### Parsing text in memory
```ParseText``` reads a text in memory from a pointer and a length. The text needs no terminating zero, and it is not copied:
```cxx
auto parseResult = jsonStructuredData.ParseText( text, length );
```

### Parse options
```Parse``` and ```ParseText``` take an optional ```Json::ParseOptions```:
```cxx
Json::ParseOptions options;
options.duplicateNames  = Json::DuplicateNames::kKeepLast;
options.maxNestingDepth = 64;

auto parseResult = jsonStructuredData.Parse( "Data/Examples/simple.json", options );
```
* ```duplicateNames``` decides what happens when an object repeats a name. ```kReject``` (the default) fails with ```Status::kDuplicateName```, ```kKeepFirst``` keeps the first value and ```kKeepLast``` keeps the last one. A kept name stays where it first appeared.
* ```maxNestingDepth``` limits how deeply arrays and objects may nest. Deeper documents fail with ```Status::kMaximumNestingDepthExceeded``` instead of exhausting the stack. The default is ```JSON4C4_MAX_NESTING_DEPTH``` (256).
* ```maxValueCount``` limits how many values (objects, arrays, strings, numbers, ```true```, ```false``` and ```null```) a document may hold, and ```maxStringByteCount``` how many bytes its names and strings may hold together, counted after their escapes are decoded. A document past either limit fails with ```Status::kMaximumValueCountExceeded``` or ```Status::kMaximumStringByteCountExceeded``` before the value or string past the limit is allocated. Neither is limited by default, and a negative limit allows nothing.

### Numbers
Numbers are read correctly rounded and written in the shortest form that reads back to the same value, with the same results in every build and locale. Values from 1e-6 up to 1e21 are written as plain decimals and others in exponent form, for example ```1e+21``` and ```1e-7```.

A ```float``` is written in its shortest form, so ```0.1f``` is written as ```0.1``` rather than ```0.10000000149011612```, and every ```float``` written reads back into a ```float``` as the same ```float```. A number is read into a ```float``` through a ```double```, which rounds it twice: the shortest form of ```7.038531e-26f``` would read back as the next ```float```, so that one is written as a ```double```, and a longer number written elsewhere can, rarely, read one step away from the nearest ```float```. A number beyond the range of ```float``` fails to validate or deserialize into one with ```Status::kFloatInvalid```.

## Documents and values
A ```Json::StructuredData``` holds a document, and ```GetRoot()``` gives its root ```Json::Value```, or ```nullptr``` when it holds none. A ```Value``` tells its type with ```GetType()``` and gives its contents with ```AsBool()```, ```AsNumber()```, ```AsString()```, ```AsArray()``` and ```AsObject()```, which give ```nullptr``` or an empty view for a value of another type. ```IsNull()``` tests for ```null```, ```IsInteger()``` for a number written without a fraction or an exponent, and ```IsFloat()``` for a number built from a ```float```.
```cxx
Json::ObjectView box = jsonStructuredData.GetRoot()->AsObject();

if ( const double* width = box.FindNumber( "width" ) )
{
    printf( "Width: %f\n", *width );
}

for ( const Json::Member& member : box )
{
    Json::StringView name = member.GetName();

    printf( "%.*s\n", int( name.GetLength() ), name.GetText() );
}
```
* An ```ObjectView``` gives its members in document order and finds them with ```Find```, ```FindBool```, ```FindNumber```, ```FindString```, ```FindArray```, ```FindObject``` and ```FindNull```. Objects of more than 16 members find a name through a sorted index.
* An ```ArrayView``` gives its elements with ```[ index ]``` and in a range-based ```for```, and ```GetElementCount()``` counts them.
* A ```StringView``` is a pointer and a length, without a terminating zero. A string may hold zeros, written ```\u0000``` in JSON. A ```std::string``` keeps them, while a ```String<>```, or a name in an ```ObjectMap```, cannot hold them, and validating or deserializing one fails with ```Status::kStringHoldsZero```.

A document keeps everything it holds in a few large blocks, which it takes from a ```Json::BlockSource``` and hands back when it is cleared with ```Clear()```, parsed again, replaced by ```SerializeFrom``` or destroyed. Values, strings and views taken from a document are valid until then, and finding or iterating allocates nothing. A value takes 16 bytes and an object member 32, so the values of a long array of small numbers, such as ```[1,1,1,...]```, take 8 times the size of its text, and the document takes about 10 times.

The default block source uses ```operator new```. A source of your own is given to the constructor and must outlive the document. It may refuse a block by returning ```nullptr```. A block the document only wanted for growth is then asked for again at smaller sizes, and when a block it needs is refused, the parse or edit fails with ```Status::kAllocationRefused```: a parse leaves the document empty, and an edit leaves it as it was.

## Writing, serializing and editing
```Write``` writes a whole document into a file, or at the end of an ```Array<char>```, indented by two spaces unless told otherwise:
```cxx
jsonStructuredData.Write( "Data/Examples/simpleRewrite.json" );
jsonStructuredData.Write( "Data/Examples/simpleTabs.json", 1, '\t' );
```
```SerializeFrom``` replaces the document with what a structure serializes to, through its process rules. A NaN or an infinity fails with ```Status::kFloatInvalid``` and a name used twice in one object with ```Status::kDuplicateName```, and a failure leaves the document empty.
```cxx
textBox.width = 2;

if ( jsonStructuredData.SerializeFrom( textBox ) == Json::Status::kOk )
{
    jsonStructuredData.Write( "Data/Examples/simpleModified.json" );
}
```
A document can also be edited in place. An edit is given a ```Json::EditHandle```, which ```Edit``` takes from a value of the document, and it builds the new value before it changes anything, so a failed edit leaves the document as it was, and the new value may be built from the value it replaces or joins.
* ```SerializeAt( handle, data )``` replaces a value.
* ```AppendElement( handle, data )``` adds an element at the end of an array.
* ```AddMember( handle, name, data )``` adds a member at the end of an object. A name the object already has fails with ```Status::kDuplicateName```.
* ```RemoveElement( handle, index )``` and ```RemoveMember( handle, name )``` remove an element or a member, and fail with ```Status::kIndexOutOfRange``` or ```Status::kNameNotPresent``` when there is none.
* ```EmptyObject( handle )``` removes every member of an object.

The ```data``` of an edit is anything that serializes: a ```double```, a ```float```, a ```bool```, a string, an array, a structure with process rules, ```Json::Null```, or a ```Json::Value``` of this document or another, which is copied.
```cxx
const Json::Value& root   = *jsonStructuredData.GetRoot();
Json::EditHandle   object = jsonStructuredData.Edit( root );

jsonStructuredData.AddMember( object, "depth", 0.5 );
jsonStructuredData.RemoveMember( object, "height" );

Json::EditHandle text = jsonStructuredData.Edit( *root.AsObject().Find( "text" ) );

jsonStructuredData.SerializeAt( text, "Hello" );
```
An edit moves the elements or members of the array or object it changes, so pointers to its elements or members, and views into them, are not valid after the edit. The array or object itself stays where it is, and a view of it stays valid unless ```SerializeAt``` gives it another type. A handle is kept up to date by the edits it is given, and any other change to the document, an edit through another handle, a parse, a ```SerializeFrom``` or a ```Clear```, makes it stale: an edit through it then fails with ```Status::kStaleEditHandle```, and its ```Get()``` gives ```nullptr```. Take a handle from a value just found, since a pointer kept across an edit can point at another value. Memory that a removal frees, or that an array or object outgrows, comes back when the document is cleared.

### Serializing your own types
A ```Serialize( Json::Node, const T& )``` function, or a ```Serialize( Json::Node ) const``` member, writes a type that has no process rules. A ```Node``` is a value being built: ```SetNull```, ```SetBool```, ```SetNumber```, ```SetFloat```, ```SetString```, ```SetArray``` and ```SetObject``` give it its value, and ```AppendElement``` and ```AddMember``` give the node of a new element or member of an array or object. The first call that fails fails the whole build, so a function may check only the status of its last call:
```cxx
Json::Status Serialize( Json::Node node, const Point& point )
{
    Json::Node x;
    Json::Node y;

    node.SetArray();
    node.AppendElement( x );
    x.SetNumber( point.x );
    node.AppendElement( y );

    return y.SetNumber( point.y );
}
```
A ```Serialize``` function must leave the document being built alone: an edit, a ```SerializeFrom``` or a parse of it during the build fails with ```Status::kBuildInProgress```, and a ```Clear``` of it does nothing.

## Versions
### Version 3.0.1
* ```SerializeFrom``` may be given a value of the document itself, as in ```doc.SerializeFrom( *doc.GetRoot()->AsObject().Find( "keep" ) )```, ```ParseText``` a string of it, and ```Parse``` a file named by one. 3.0 read such a value from memory the document had already handed back. A ```SerializeFrom``` now holds the old document until the new one is built.
* A ```Serialize``` that goes on after an array failed to close, as when a block source refuses its memory, no longer crashes the build.
* ```AddMember``` with a null name fails with ```Status::kNameNotPresent```, through an ```EditHandle``` or a ```Node```, instead of adding a member named ```""```, and ```AddMember``` and ```RemoveMember``` given a null name and no length no longer read through it.
* ```Write``` fails with ```Status::kFileTooLarge``` on a string of 2 GiB or more instead of crashing, and serializing a ```std::string``` of 4 GiB or more fails with ```Status::kMaximumStringByteCountExceeded``` instead of keeping only part of it.
* Writing with gcc is about as fast as in 2.3 again. 3.0 wrote up to 1.76 times slower with gcc, which copied the growing output one byte at a time and read the last characters of each string through a call to ```memcpy```.
* A view of a value that ```SerializeAt``` replaces stays valid only when the new value has the same type. 3.0 promised it in every case, but a view of an array or object replaced by a value of another type reads what is no longer there.

### Version 3.0
Version 3.0 replaces the value classes of 2.x with plain values read through views, and adds editing in place, ```Json::Read```, block sources and ```float``` members written in their shortest form. Parsing is faster. Code that parses, validates, deserializes, serializes and writes through the ```DEFINE_JSON4C4_FUNCTIONS``` and ```DEFINE_JSON4C4_MEMBER_FUNCTIONS``` macros needs no change. Code that reads values or defines its own functions changes as follows:

| 2.x | 3.0 |
|---|---|
| ```GetRootJsonValue()``` | ```GetRoot()```, a ```const Value*``` |
| ```StringValue```, ```NumberValue```, ```BoolValue```, ```NullValue```, ```ArrayValue```, ```ObjectValue``` | ```Value``` and ```GetType()```, with ```StringView```, ```ArrayView```, ```ObjectView``` and ```Member``` |
| ```GetStringData()```, ```GetDoubleData()```, ```GetBoolData()```, ```GetJsonNullData()``` | ```AsString()```, ```AsNumber()```, ```AsBool()```, ```IsNull()``` |
| ```GetJsonValueArrayData()```, ```AsJsonObjectValue()```, ```GetDataAsPointerTo<T>()``` | ```AsArray()```, ```AsObject()``` |
| ```Value::name```, ```GetKey()``` | ```Member::GetName()``` |
| ```FindValue```, ```FindMapElement``` | ```ObjectView::Find``` |
| ```FindString```, ```FindNumber```, ```FindBool``` | the same on ```ObjectView```, where ```FindString``` gives a ```StringView``` |
| ```FindJsonNull```, ```FindJsonValueArray```, ```FindJsonObjectValue``` | ```FindNull```, ```FindArray```, ```FindObject``` |
| ```Value::Write``` | ```StructuredData::Write``` into an ```Array<char>```, for whole documents |
| ```InsertAccountedMapElement```, ```RemoveAccountedMapElement```, ```PurgeAccountedMap```, ```Value::Detach``` | ```AddMember```, ```RemoveMember```, ```EmptyObject```, ```RemoveElement``` on the document, through an ```EditHandle``` |
| ```Validate( const Value*, const T& )```, ```Deserialize( const Value*, T& )``` | ```Validate( const Value&, const T& )```, ```Deserialize( const Value&, T& )``` |
| ```ValidateProto( value, ... )```, ```DeserializeProto( value, ... )``` | ```ValidateProto( value, path, ... )``` and ```DeserializeProto( value, path, ... )```, in ```ValidateTraced``` and ```DeserializeTraced``` |
| ```Serialize( Value*, const T& )```, ```SerializeProto( value, ... )``` | ```Serialize( Node, const T& )```, ```SerializeProto( node, ... )``` |
| ```MayThrow``` | ```Detail::MayThrow``` |
| ```Status::kExpectedBeginingDoubleQuotes``` | ```Status::kExpectedBeginningDoubleQuotes``` |

A ```Validate``` or ```Deserialize``` function of your own that takes a ```const Value&``` still works, and adds nothing to the path of a failure. Behaviour changes:
* An optional member that is present must be valid as a whole. 2.x ignored a member missing inside it, so it could succeed and leave an optional array empty.
* Strings may hold ```\u0000```, which 2.x refused with ```Status::kStringIllegalEscape```. Validating or deserializing such a string into a ```String<>``` fails with ```Status::kStringHoldsZero```.
* ```SerializeFrom``` fails on a NaN or an infinity, which 2.x refused only when writing, and a failed ```SerializeFrom``` leaves the document empty.
* ```kAllocationRefused```, ```kStringHoldsZero```, ```kIndexOutOfRange```, ```kStaleEditHandle``` and ```kBuildInProgress``` are new statuses, and the unused ```kMissingObjectElement``` is gone, so the statuses after it are numbered one lower. Two status strings now say square brackets instead of square braces, and one spells beginning correctly.

### Version 2.3
* ```ParseText``` reads a text in memory from a pointer and a length, without a terminating zero and without copying it.
* ```ParseOptions::maxValueCount``` and ```ParseOptions::maxStringByteCount``` limit how many values and string bytes a document may hold, with the new statuses ```kMaximumValueCountExceeded``` and ```kMaximumStringByteCountExceeded```.
* Objects with more than 16 members find names through a sorted index instead of a hash, so no choice of names can make parsing or finding a member slow. In such an object a repeated name is found once the whole object has been read, so any error later in the same object, one inside a nested value included, is reported instead of ```kDuplicateName```.
* MSVC builds without exceptions contain no ```try``` or ```catch```.
* ```ParseOptions``` is larger, so code built against the 2.2 header must be rebuilt to use a 2.3 shared library.
* ```JSON4C4_MAX_FILE_SIZE``` is 0x7FFFFF00 by default and can no longer be defined higher, so that the size of every string of a text fits in 32 bits.

### Version 2.2.1
* Numbers are converted with the NumberTextThree header, which replaces NumberText. Inside the C4 Engine, copy ```NumberTextThree.h``` from ```Code/NumberTextThree``` instead of ```NumberText.h```. Numbers are read faster, and read and write exactly as before.

### Version 2.2
* Parsing and writing are faster, most of all for documents with many strings and objects.
* Objects keep their members in document order: iterating over an ```ObjectValue``` visits them in the order they were read or inserted, the order ```Write``` uses.
* ```Value``` is no longer a Terathon ```MapElement```. A member still leaves its object when it is deleted, detached with ```Value::Detach``` or inserted into another object.
* ```InsertAccountedMapElement``` allocates and is no longer ```noexcept```.
* ```ArrayValue::Parse``` and ```ObjectValue::Parse``` are no longer public, and values can no longer be copied.

### Version 2.1
* Numbers are converted with the NumberText header in every build, including inside the C4 Engine, so parsing and writing numbers is much faster and gives the same results everywhere.
* Outside the C4 Engine, the layout of written numbers changes for very large and very small values: for example ```1e+15``` is now written as ```1000000000000000``` and ```1e-05``` as ```0.00001```. The values read back exactly as before.
* ```JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS``` no longer has any effect.
* The installed CMake package exports the ```Json4C4::Json4C4``` target, as the documentation always said.

### Version 2.0
Version 2.0 changes the following, and code written for 1.x may need updating:
* The file status codes are now ```kFileOpenError```, ```kFileReadError``` and ```kFileTooLarge```. ```kMaximumNestingDepthExceeded```, ```kDuplicateName``` and ```kFileWriteError``` are new.
* Duplicate names are rejected by default (see ```ParseOptions```).
* A failed ```Parse``` clears the previously loaded document.
* Deserializing into an ```ObjectMap``` or ```std::map``` replaces its contents. On failure, arrays, vectors and ```ObjectMap```s are left empty and a ```std::map``` is left unchanged.
* ```Write``` refuses NaN and infinity (```Status::kFloatInvalid```) and strings that are not valid UTF-8 (```Status::kStringIllegalChar```). Outside the C4 Engine it writes to a temporary file and replaces the target only when everything was written, so a failed write leaves an existing file untouched. Symbolic links and files with more than one hard link are written in place instead.
* Numbers are written in the shortest form that reads back to the same value.
* ```ObjectValue``` no longer exposes its map publicly. Use ```FindValue```, ```InsertAccountedMapElement```, ```RemoveAccountedMapElement``` and ```PurgeAccountedMap```.
* ```Value::Write``` writes into an ```Array<char>```.

## Integrating into the C4 Engine Visual Studio solution
First, copy the ```C4Json.h``` and ```C4Json.cpp``` files from the ```Code/Json4C4``` subfolder and the ```NumberTextThree.h``` file from the ```Code/NumberTextThree``` subfolder into the C4 Engine ```EngineCode``` directory. Subsequently, add the three files to the ```Engine``` project by right-clicking on the ```System``` filter under the ```Engine``` project in the ```Solution Explorer``` and selecting ```Add->Existing Item```.

Include by ```#include <C4Json.h>``` to use from your application code. Note that this is different from when using the library outside the C4 Engine, where you need ```#include <Json4C4/C4Json.h>```.

## Incorporating in any regular (non C4-Engine) C++ project using cmake
The easiest way to incorporate Json4C4 into any C++ project that uses cmake is by the ```FetchContent``` commands. Cmake will download, compile, and make the include directories and the Json4C4 library available to your project. An example ```CMakeLists.txt``` file is shown below.
```cmake
include( FetchContent )

# Optional: uncomment the following line if you don't want std::string, std::vector and std::map support.
# set( Json4C4EnableStdSupport No CACHE INTERNAL "" )  

FetchContent_Declare(
  Json4C4
  GIT_REPOSITORY "https://github.com/nasosi/Json4C4"
  GIT_TAG v3.0.1 # Or whatever version you select
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable( Json4C4 )

add_executable( exampleTarget main.cpp )

target_link_libraries( exampleTarget PRIVATE Json4C4::Json4C4 )
```
An  example can be found in ```Code/Examples/Example07_cmake_fetchcontent```.

If you prefer to have a local copy, you can alternatively clone Json4C4 and use the local directory instead of the GitHub URL. For example:
```cmake
...
GIT_REPOSITORY "C:/users/joe/codes/Json4C4"
...
```
## Using in projects that do not utilize Terathon data structures
### ```std::``` data structures
Json4C4 supports ```std::string```, ```std::vector```, and ```std::map``` by default in the standalone compilation mode (when ```TERATHON_NO_SYSTEM``` is not defined). If you want to disable these ```std``` data structures, you have two options:
* If you are using cmake, compile Json4C4 with the ```Json4C4EnableStdSupport``` cmake argument set to ```No```.
* If you are not using cmake, define ```JSON4C4_DISABLE_STD_SUPPORT``` for compiling ```C4Json.cpp``` or including ```C4Json.h```.

### Custom string and array data structures
If you want to use custom string and array data structures, you have two options:
1. Implement ```ValidateTraced```, ```DeserializeTraced``` and ```Serialize``` overloads for them. Look in ```C4Json.h``` and ```C4Json.cpp``` inside the code enabled by the ```JSON4C4_ENABLE_STD_SUPPORT_INTERNAL```  for an example of how this is implemented for ```std::string```, ```std::vector```, and ```std::map```, or
2. Manually perform deserialization and populate your data accordingly. ```Example05_manual_deserialization``` demonstrates how to achieve this.


## Preprocessor macros
Json4C4 uses several preprocessor macros explained below.

### ```DEFINE_JSON4C4_FUNCTIONS```
```DEFINE_JSON4C4_FUNCTIONS``` is a convenience macro that enables defining the three core functions (```ValidateTraced```, ```DeserializeTraced```, and ```Serialize```) needed to perform the relevant operations on custom user types.

### ```DEFINE_JSON4C4_MEMBER_FUNCTIONS```
```DEFINE_JSON4C4_MEMBER_FUNCTIONS``` is a convenience macro that enables defining the three core *member* functions (```ValidateTraced```, ```DeserializeTraced```, and ```Serialize```) needed to perform the relevant operations on custom user types, as part of a ```class``` or ```struct```. Its main purpose is to allow for validating, deserializing, and serializing member values that are private to the ```class``` or ```struct```.

### ```JSON4C4_DISABLE_STD_SUPPORT```
When ```JSON4C4_DISABLE_STD_SUPPORT``` is defined, or when ```TERATHON_NO_SYSTEM``` is defined, Json4C4 will be compiled without ```std::string```, ```std::vector```, and ```std::map``` support.

### ```JSON4C4_MAX_NESTING_DEPTH```
The default for ```ParseOptions::maxNestingDepth```, 256 unless defined otherwise.

### ```JSON4C4_MAX_VALUE_COUNT``` and ```JSON4C4_MAX_STRING_BYTE_COUNT```
The defaults for ```ParseOptions::maxValueCount``` and ```ParseOptions::maxStringByteCount```, no limit (0x7FFFFFFFFFFFFFFF) unless defined otherwise.

### ```JSON4C4_MAX_FILE_SIZE```
The largest file or text ```Parse```, ```ParseText``` and ```Read``` read and ```Write``` produces, 0x7FFFFF00 bytes unless defined lower. A longer text fails with ```Status::kFileTooLarge```.

## Third party licenses
Third-party licenses are located in the ```Licenses``` subfolder.
