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

#include <cfloat>
#include <cstdio>

namespace Json = C4::Json;
namespace TS   = Terathon;

// Example 03: Same as example02, but without macros.

struct Mechanical
{
    double            horsepower = 0;
    bool              automatic  = 0;
    TS::Array<double> gearRatios;
};

inline Json::Status Deserialize( const Json::Value* value, Mechanical& o ) noexcept
{
    return Json::DeserializeProto( value, "horsepower", o.horsepower, "gearRatios", o.gearRatios, "automatic", o.automatic );
}
inline Json::Status Validate( const Json::Value* value, const Mechanical& o ) noexcept
{
    return Json::ValidateProto( value, "horsepower", o.horsepower, "gearRatios", o.gearRatios, "automatic", o.automatic );
}

inline Json::Status Serialize( Json::Value* value, const Mechanical& o ) noexcept
{
    return Json::SerializeProto( value, "horsepower", o.horsepower, "gearRatios", o.gearRatios, "automatic", o.automatic );
}

// If some members are private, the prototype functions need to be defined inside the class
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

    inline Json::Status Deserialize( const Json::Value* value ) noexcept
    {
        return Json::DeserializeProto(
            value, "make", make, "model", model, "rgbColor", color, "length", length, "width", width, "mechanical", mechanical, "price$", priceDollars );
    }

    inline Json::Status Validate( const Json::Value* value ) const noexcept
    {
        return Json::ValidateProto(
            value, "make", make, "model", model, "rgbColor", color, "length", length, "width", width, "mechanical", mechanical, "price$", priceDollars );
    }

    inline Json::Status Serialize( Json::Value* value ) const noexcept
    {
        return Json::SerializeProto(
            value, "make", make, "model", model, "rgbColor", color, "length", length, "width", width, "mechanical", mechanical, "price$", priceDollars );
    }
};

int main()
{
    printf( "Json4C4 version %s.\n", Json::GetVersion() );
    printf( "Example03 : Same as example02, but without macros.\n" );

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

    // Rewrite the same structure
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
