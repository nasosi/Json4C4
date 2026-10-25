<!--
  Title: Json4C4
  Description: general-purpose JSON library developed to integrate well with the C4 Engine
  Author: Nasos
-->

# Json4C4
Json4C4 is a general-purpose JSON library developed to integrate well with the [C4 Engine](https://c4engine.com/). Json4C4 can also be used in any C++ application if desired.

Json4C4 is appropriate for parsing, deserialization, serialization, and validation. The library provides ways for the simple definition of the data structure prototype to enable seamless interoperability with user-defined objects. It is compliant with the [RFC 8259 (STD90) standard](https://datatracker.ietf.org/doc/html/rfc8259) and supports accurate JSON file syntax-error reporting. It is fast, and its number conversions are exact and identical in every build.

Json4C4  has no external dependencies. It internally uses parts of the [Terathon Container](https://github.com/EricLengyel/Terathon-Container-Library), the [OpenDDL Reference Implementation](https://github.com/EricLengyel/OpenDDL) and  [Terathon Math](https://github.com/EricLengyel/Terathon-Math-Library) libraries, and the NumberText header for number conversions.

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
Json::Status Deserialize( const Json::Value* value, TextBox& object )
{
    return Json::DeserializeProto( value,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
	);
}

Json::Status Serialize( Json::Value* value, const TextBox& object )
{
    return Json::SerializeProto( value,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
	);
}

Json::Status Validate( const Json::Value* value, const TextBox& object )
{
    return Json::ValidateProto( value,
        "text",   object.text,
        "width",  object.width,
        "height", object.height
	);
}
```
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
If the ```Deserialize``` call is not  successful, the ```textBox``` object is in an undefined state. If, in your code, an undefined state is not handled, then you likely need to validate the structured data before deserializing. ```Example02_non_trivial_data_structure``` shows how to achieve this, along with demonstrating additional features of the Json4C4 library.

### Optional arguments
For optional arguments, you can use the keyword ```Json::optional``` to designate them as such before the field name. For example, in the following, absence of ```height``` in the JSON file will not result in an error and the relevant member will be ignored:
```cxx
#define TEXTBOX_PROTO \
    "text",   object.text,\
    "width",  object.width,\
    Json::optional, "height", object.height
```

### Parse options
Both ```Parse``` overloads take an optional ```Json::ParseOptions```:
```cxx
Json::ParseOptions options;
options.duplicateNames  = Json::DuplicateNames::kKeepLast;
options.maxNestingDepth = 64;

auto parseResult = jsonStructuredData.Parse( "Data/Examples/simple.json", options );
```
* ```duplicateNames``` decides what happens when an object repeats a name. ```kReject``` (the default) fails with ```Status::kDuplicateName```, ```kKeepFirst``` keeps the first value and ```kKeepLast``` keeps the last one. A kept name stays where it first appeared.
* ```maxNestingDepth``` limits how deeply arrays and objects may nest. Deeper documents fail with ```Status::kMaximumNestingDepthExceeded``` instead of exhausting the stack. The default is ```JSON4C4_MAX_NESTING_DEPTH``` (256).

### Numbers
Numbers are read correctly rounded and written in the shortest form that reads back to the same value, with the same results in every build and locale. Values from 1e-6 up to 1e21 are written as plain decimals and others in exponent form, for example ```1e+21``` and ```1e-7```.

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
First, copy the ```C4Json.h``` and ```C4Json.cpp``` files from the ```Code/Json4C4``` subfolder and the ```NumberText.h``` file from the ```Code/NumberText``` subfolder into the C4 Engine ```EngineCode``` directory. Subsequently, add the three files to the ```Engine``` project by right-clicking on the ```System``` filter under the ```Engine``` project in the ```Solution Explorer``` and selecting ```Add->Existing Item```.

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
  GIT_TAG v2.1.0 # Or whatever version you select
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
1. Implement appropriate overloads. Look in ```C4Json.h``` and ```C4Json.cpp``` inside the code enabled by the ```JSON4C4_ENABLE_STD_SUPPORT_INTERNAL```  for an example of how this is implemented for ```std::string```, ```std::vector```, and ```std::map```, or
2. Manually perform deserialization and populate your data accordingly. ```Example05_manual_deserialization``` demonstrates how to achieve this.


## Preprocessor macros
Json4C4 uses several preprocessor macros explained below.

### ```DEFINE_JSON4C4_FUNCTIONS```
```DEFINE_JSON4C4_FUNCTIONS``` is a convenience macro that enables defining the three core functions (```Validate```, ```Deserialize```, and ```Serialize```) needed to perform the relevant operations on custom user types.

### ```DEFINE_JSON4C4_MEMBER_FUNCTIONS```
```DEFINE_JSON4C4_MEMBER_FUNCTIONS``` is a convenience macro that enables defining the three core *member* functions (```Validate```, ```Deserialize```, and ```Serialize```) needed to perform the relevant operations on custom user types, as part of a ```class``` or ```struct```. Its main purpose is to allow for validating, deserializing, and serializing member values that are private to the ```class``` or ```struct```.

### ```JSON4C4_DISABLE_STD_SUPPORT```
When ```JSON4C4_DISABLE_STD_SUPPORT``` is defined, or when ```TERATHON_NO_SYSTEM``` is defined, Json4C4 will be compiled without ```std::string```, ```std::vector```, and ```std::map``` support.

### ```JSON4C4_MAX_NESTING_DEPTH```
The default for ```ParseOptions::maxNestingDepth```, 256 unless defined otherwise.

### ```JSON4C4_MAX_FILE_SIZE```
The largest file ```Parse``` reads and ```Write``` produces, 0x7FFFFFFE bytes unless defined otherwise.

## Third party licenses
Third-party licenses are located in the ```Licenses``` subfolder.
