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

// Example 05: Simple parsing and manual json structure query.

int main()
{
    printf( "Json4C4 version %s.\n", Json::GetVersion() );
    printf( "Example05 : Simple parsing and manual JSON structure query.\n" );

    // Parse the file and check for parsing errors.
    Json::StructuredData jsonStructuredData;
    auto                 parseResult = jsonStructuredData.Parse( "Data/Examples/simple.json" );

    if ( parseResult.status != Json::Status::kOk )
    {
        TS::String<256> errorDescription = Json::ParseResultToString( parseResult );

        fprintf( stderr, "JSON parse error: %s\n", static_cast<const char*>( errorDescription ) );

        return 1;
    }

    // Get the root json value
    Json::Value* root = jsonStructuredData.GetRootJsonValue();

    // Cast it to a Json::ObjectValue, which is essentially a map.
    Json::ObjectValue* objectValue = root->AsJsonObjectValue();
    if ( !objectValue )
    {
        fprintf( stderr, "Error: expected a Json::ObjectValue as the root of the parsed data structure.\n" );
        return 1;
    }

    // Get the value of key "width". If the query was not succesful exit.
    const double* width = objectValue->FindNumber( "width" );
    if ( !width )
    {
        fprintf( stderr, "Error: expected a number value for the item with name \"width\".\n" );
        return 1;
    }

    printf( "Parsed width: %f\n", *width );

    return 0;
}
