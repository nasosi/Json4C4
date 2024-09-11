# Json4C4
Json4C4 is a general-purpose JSON library developed to integrate well with the [C4 Engine](https://c4engine.com/). Json4C4 can also be used in any C++ application if desired.

Json4C4 is appropriate for parsing, deserialization, serialization, and validation. The library provides ways for the simple definition of the data structure prototype to enable seamless interoperability with user-defined objects. It is compliant with the [RFC 8259 (STD90) standard](https://datatracker.ietf.org/doc/html/rfc8259) and supports accurate JSON file syntax-error reporting.

Json4C4  has no external dependencies. It internally uses parts of the [Terathon Container](https://github.com/EricLengyel/Terathon-Container-Library), the [OpenDDL Reference Implementation](https://github.com/EricLengyel/OpenDDL) and  [Terathon Math](https://github.com/EricLengyel/Terathon-Math-Library) libraries.

## Simple example
The use of Json4C4 includes three main steps. Assume for example a ```struct``` like the following:
```cxx
struct TextBox
{
    Terathon::String<> text;
    double width = 0;
    double height = 0;
};
```
First, you need to define the process rules for deserialization/serialization/validation:
```cxx
bool Deserialize(const Json::Value* value, TextBox& object)
{
    return Json::DeserializeProto(value,
        "text", object.text,
        "width", object.width,
        "height",  object.height
	);
}

bool Serialize(Json::Value* value, const TextBox& object)
{
    return Json::SerializeProto(value,
        "text", object.text,
        "width", object.width,
        "height", object.height
	);
}

bool Validate(const Json::Value* value, const TextBox& object)
{
    return Json::ValidateProto(value,
        "text", object.text,
        "width", object.width,
        "height", object.height
	);
}
```
Alternatively, you can use Json4C4's ```DEFINE_JSON4C4_FUNCTIONS``` macro, which generates all three definitions:
```cxx
#define TEXTBOX_PROTO \
    "text", object.text,\
    "width", object.width,\
    "height", object.height

DEFINE_JSON4C4_FUNCTIONS(TextBox, TEXTBOX_PROTO)
```
Second, you create a ```Json::StructuredData``` and parse the file:
```cxx
Json::StructuredData jsonStructuredData;
auto parseResult = jsonStructuredData.Parse("Data/Examples/simple.json");
```
And last, you deserialize the structure. If this call is successful, the data of the file has been assigned to the object member variables:
```cxx
if (jsonStructuredData.DeserializeTo(textBox))
{
    fprintf(stderr, "Deserialization error\n");
    return 1;
}

printf("TextBox text: %s. Width: %f, height: %f\n", static_cast<const char*>(textBox.text), textBox.width, textBox.height);
```
If the ```Deserialize``` call is not  successful, the ```textBox``` object is in an undefined state. If, in your code, an undefined state is not handled, then you likely need to validate the structured data before deserializing. ```Example02_non_trivial_data_structure``` shows how to achieve this, along with demonstrating additional features of the Json4C4 library.

### Optional arguments (added in version 1.1.1)
For optional arguments, you can use the keyword optional to designate them as such before the field name. For example:
```cxx
#define TEXTBOX_PROTO \
    "text", object.text,\
    "width", object.width,\
    Json::optional, "height", object.height
```
## Integrating into the C4 Engine Visual Studio solution
First, copy the ```Json4C4.h```and ```Json4C4.cpp``` files from the ```Code/Json4C4``` subfolder into the C4 Engine ```EngineCode``` directory. Subsequently, add the two files to the ```Engine``` project by right-clicking on the ```System``` filter under the ```Engine``` project in the ```Solution Explorer``` and selecting ```Add->Existing Item```.

Include by ```#include <C4Json.h>``` to use from your application code. Note that this is different from when using the library outside the C4 Engine, where you need ```#include <Json4c4/C4Json.h>```.

## Incorporating in any regular (non C4-Engine) C++ project using cmake
The easiest way to incorporate Json4C4 into any C++ project that uses cmake is by the ```FetchContent``` commands. Cmake will download, compile, and make the include directories and the Json4C4 library available to your project. An example ```CMakeLists.txt``` file is shown below.
```cmake
include( FetchContent )

# Optional: uncomment the following line if you don't want std::string and std::vector support.
# set(Json4C4EnableStdSupport No CACHE INTERNAL "")  

FetchContent_Declare(
  Json4C4
  GIT_REPOSITORY "https://github.com/nasosi/Json4C4"
  GIT_TAG v1.1.1 # Or whatever version you select
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
* If using cmake, compile Json4C4 with the ```Json4C4EnableStdSupport``` cmake argument set to ```No```.
* If not using cmake, define ```JSON4C4_DISABLE_STD_SUPPORT``` for compiling ```Json4C4.cpp``` or including ```Json4C4.hpp```.

### Custom string and array data structures
If you want to use custom string and array data structures, you have two options:
1. Implement appropriate overloads. Look in ```C4Json.h``` and ```C4Json.cpp``` inside the code enabled by the ```JSON4C4_ENABLE_STD_SUPPORT_INTERNAL```  for an example of how this is implemented for ```std::string```, ```std::vector```, and ```std::map```.
2. Manually perform deserialization and populate your data accordingly. ```Example05_manual_deserialization``` demonstrates how to achieve this.


## Preprocessor macros
Json4C4 uses three preprocessor macros explained below.

### ```DEFINE_JSON4C4_FUNCTIONS```
```DEFINE_JSON4C4_FUNCTIONS``` is a convenience macro that provides the definition of the three core functions (```Validate```, ```Deserialize```, and ```Serialize```) needed to perform the relevant operations on custom user types.

### ```DEFINE_JSON4C4_MEMBER_FUNCTIONS```
```DEFINE_JSON4C4_MEMBER_FUNCTIONS``` is a convenience macro that provides the definition of the three core *member* functions (```Validate```, ```Deserialize```, and ```Serialize```) needed to perform the relevant operations on custom user types, as part of a ```class``` or ```struct```. Its main purpose is to allow for validating, deserializing, and serializing member values that are private to the ```class``` or ```struct```.

### ```JSON4C4_DISABLE_STD_SUPPORT```
When ```JSON4C4_DISABLE_STD_SUPPORT``` is defined, or when ```TERATHON_NO_SYSTEM``` is defined, Json4C4 will be compiled without ```std::string```, ```std::vector```, and ```std::map``` support.

### ```JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS```
Because C4 Engine implements many system functions, it, by default, does not support system libraries. This can be turned off by undefining ```TERATHON_NO_SYSTEM``` when compiling the C4 Engine. 

Because some system functions enable higher accuracy parsing of floating numbers, Json4C4 provides the ```JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS``` define. If it is defined, then Json4C4 will use [strod](https://en.cppreference.com/w/c/string/byte/strtof) to convert a string to the ```double``` type and attain accurate parsing. Otherwise, Json4C4 will use an implementation inspired by the following [OpenDDL](https://github.com/EricLengyel/OpenDDL) implementation, which is somewhat less accurate for edge cases. The inaccuracies are at a level that should not be of concern for most types of computations.

In the standalone mode, where ```TERATHON_NO_SYSTEM``` is not defined, Json4C4 uses high-accuracy conversions.

## Third party licenses
Third-party licenses are located in the ```Licenses``` subfolder.
