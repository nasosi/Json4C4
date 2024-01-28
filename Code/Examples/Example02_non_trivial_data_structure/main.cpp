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

#include "Json4C4/C4Json.h"

#include <cfloat>
#include <cstdio>

namespace Json = C4::Json;
namespace TS   = Terathon;

// Example 02: Parsing, validating, deserializing and serializing a class. The "Car" class has some member variables
// and also has another struct "Mechanical" as a member, that has its own members.

struct Mechanical
{
    double            horsepower = 0;
    bool              automatic  = 0;
    TS::Array<double> gearRatios;
};

// Define process rules for deserialization / serialization / validation.
#define MECHANICAL_PROTO "horsepower", object.horsepower, "gearRatios", object.gearRatios, "automatic", object.automatic

DEFINE_JSON4C4_FUNCTIONS( Mechanical, MECHANICAL_PROTO )

// If some members are private, the prototype needs to be defined inside the class using the DEFINE_JSON4C4_MEMBER_FUNCTIONS macro
class Car
{
private:
    double length = 0;
    double width  = 0;

public:
    TS::String<>      make;
    TS::String<>      model;
    TS::Array<double> color;
    Mechanical        mechanical;
    double            priceDollars = 0;

#define CAR_PROTO "make", make, "model", model, "rgbColor", color, "length", length, "width", width, "mechanical", mechanical, "price$", priceDollars

    DEFINE_JSON4C4_MEMBER_FUNCTIONS( CAR_PROTO )
};

int main()
{
    printf( "Json4C4 version %s.\n", Json::GetVersion() );
    printf( "Example02 : A non trivial example involving parsing, validation, deserialization, serialization and writing to files.\n" );

    // Parse data from a file
    Json::StructuredData jsonStructuredData;
    auto                 parseResult = jsonStructuredData.Parse( "Data/Examples/car.json" );

    if ( parseResult.status != Json::Status::kOk )
    {
        TS::String<256> errorDescription = Json::ParseResultToString( parseResult );

        fprintf( stderr, "JSON parse error: %s\n", static_cast<const char*>( errorDescription ) );

        return 1;
    }

    // Create the object
    Car car;

    // Validate that the parsed Json structured data matches the process rules defined above. Then deserialize to the structure
    // If you take care for undefined -car- data after the following -if-, you could use Deserialize only.
    if ( Json::Validate( jsonStructuredData, car ) != Json::Status::kOk )
    {
        fprintf( stderr, "Structure validation error\n" );
        return 1;
    }

    // Deserialize the data to your object
    jsonStructuredData.DeserializeTo( car );

    // Display some information
    {
        TS::String<> displayString = TS::String<>( car.make ) + " " + TS::String<>( car.model ) + ". Gear ratios: ";

        for ( double gearRatio : car.mechanical.gearRatios )
        {
            displayString += TS::Text::FloatToString( gearRatio ) + " ";
        }

        printf( "Some car info:\n" );
        printf( "%s", static_cast<const char*>( displayString ) );
    }

    // Perform some checks
    {
        printf( "\n%s\n", ( DBL_MIN == car.priceDollars ? "Accurate double parsing." : "Inaccurate double parsing." ) );
        printf( "Expected: %0.28g\n", DBL_MIN );
        printf( "Parsed: %0.28g\n", car.priceDollars );
    }

    // Rewrite the same structure to a file
    {
        if ( jsonStructuredData.Write( "Data/Examples/carRewrite.json" ) != Json::Status::kOk )
        {
            fprintf( stderr, "Error writing to file.\n" );
            return 1;
        }
        printf( "Serialized data to Data/Examples/carModified.json" );
    }

    // Modify the structure and write to another file
    {
        car.priceDollars = 1;
        jsonStructuredData.SerializeFrom( car );

        if ( jsonStructuredData.Write( "Data/Examples/carModified.json" ) != Json::Status::kOk )
        {
            fprintf( stderr, "Error writing to file.\n" );
            return 1;
        }
        printf( "Wrote a modified version to Data/Examples/carModified.json" );
    }

    return 0;
}
