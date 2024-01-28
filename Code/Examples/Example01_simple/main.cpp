// Json4C4
// A C++ JSON library
//
// MIT License
//
// Copyright(c) 2024 Athanasios Iliopoulos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// This software uses third party libraries. Their licenses can be found in the
// "Licenses" subfolder of the source code repository.

#include <Json4C4/C4Json.h>

#include <cstdio>

namespace Json = C4::Json;
namespace TS   = Terathon;

// Example 01: Parsing, and deserializing data to an object.

// Assuming a structure:
struct TextBox
{
    TS::String<> text;
    double       width  = 0;
    double       height = 0;
};

// 1. Define process rules for deserialization/serialization/validation. A none-macro example is found in Exampl03_no_macros
#define TEXTBOX_PROTO "text", object.text, "width", object.width, "height", object.height

DEFINE_JSON4C4_FUNCTIONS( TextBox, TEXTBOX_PROTO )

int main()
{
    printf( "Json4C4 version %s.\n", Json::GetVersion() );
    printf( "Example01: Simple parsing and deserialization.\n" );

    // 2. Parse the file and check parsing status
    Json::StructuredData jsonStructuredData;
    auto                 parseResult = jsonStructuredData.Parse( "Data/Examples/simple.json" );

    if ( parseResult.status != Json::Status::kOk )
    {
        TS::String<256> errorDescription = Json::ParseResultToString( parseResult );

        fprintf( stderr, "JSON parse error: %s\n", static_cast<const char*>( errorDescription ) );

        return 1;
    }

    // 3. Create your object and deserialize the JSON structured data to it
    TextBox textBox;

    // Note: if this fails, "textBox" is in an undefined state. See example 02 for how to first validate.
    if ( jsonStructuredData.DeserializeTo( textBox ) != Json::Status::kOk )
    {
        fprintf( stderr, "Deserialization error\n" );
        return 1;
    }

    // Display parsed information
    printf( "TextBox text: %s. Width: %f, height: %f\n", static_cast<const char*>( textBox.text ), textBox.width, textBox.height );

    return 0;
}
