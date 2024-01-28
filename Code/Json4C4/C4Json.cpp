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

#include "C4Json.h"
#include "TSText.h"

#if defined( _MSC_VER )

#    pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#endif

#define JSON4C4VERSIONMAJOR 1
#define JSON4C4VERSIONMINOR 0
#define JSON4C4VERSIONPATCH 0

#define STRN( s ) #s
#define STR( s ) STRN( s )

#define JSON4C4VERSION STR( JSON4C4VERSIONMAJOR ) "." STR( JSON4C4VERSIONMINOR ) "." STR( JSON4C4VERSIONPATCH )

#ifndef TERATHON_NO_SYSTEM

#    ifndef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#        define JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#    endif

#endif

#ifdef C4_ENGINE_MODULE

#    include "C4Files.h"

#else

#    include <fstream>

#endif

#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS

#    include <cstdlib>
#    include <cstring>

#endif

namespace C4
{

#ifndef C4_ENGINE_MODULE

    enum FileOpenMode
    {
        kFileReadOnly,
        kFileCreate
    };

    enum FileStatus
    {
        kFileOkay,
        kFileOpenFailed,
        kFileNotOpen,
        kFileWriteError,
        kFileReadError
    };

    class File
    {
    private:
        std::fstream fileStream;
        bool         IsReadOnly = false;

    public:
        FileStatus OpenFile( const char* fileName, FileOpenMode fileOpenMode )
        {
            CloseFile();

            if ( fileOpenMode == kFileReadOnly )
            {
                fileStream.open( fileName, std::ios::in | std::ios::binary );

                FileStatus result = fileStream.is_open() ? kFileOkay : kFileOpenFailed;

                if ( result == kFileOkay )
                {
                    IsReadOnly = true;
                }

                return result;
            }

            fileStream.open( fileName, std::ios::out );

            FileStatus result = fileStream.is_open() ? kFileOkay : kFileOpenFailed;

            if ( result == kFileOkay )
            {
                IsReadOnly = false;
            }

            return result;
        }

        void CloseFile()
        {
            fileStream.close();
        }

        Terathon::uint64 GetFileSize()
        {
            if ( fileStream.is_open() )
            {
                std::streampos originalPosition = fileStream.tellg();

                fileStream.seekg( std::ios::beg, std::ios::end );

                if ( !fileStream )
                {
                    return 0;
                }

                std::streampos size = fileStream.tellg();

                if ( !fileStream )
                {
                    return 0;
                }

                fileStream.clear();

                if ( !fileStream )
                {
                    return 0;
                }

                fileStream.seekg( originalPosition );

                if ( !fileStream )
                {
                    return 0;
                }

                return size;
            }

            return 0;
        }

        FileStatus ReadFile( char* buffer, Terathon::uint64 size )
        {
            if ( !fileStream.is_open() )
            {
                return kFileNotOpen;
            }

            fileStream.read( buffer, size );

            if ( !fileStream )
            {
                return kFileReadError;
            }

            return kFileOkay;
        }

        FileStatus WriteFile( char* buffer, Terathon::uint64 size )
        {
            if ( IsReadOnly )
            {
                return kFileWriteError;
            }

            if ( !fileStream.is_open() )
            {
                return kFileNotOpen;
            }

            fileStream.write( buffer, size );

            if ( !fileStream )
            {
                return kFileWriteError;
            }

            return kFileOkay;
        }

        File& operator<<( char c )
        {
            if ( fileStream.is_open() && !IsReadOnly )
            {
                fileStream << c;
            }

            return ( *this );
        }

        File& operator<<( const char* c )
        {
            if ( fileStream.is_open() && !IsReadOnly )
            {
                fileStream.write( c, strlen( c ) );
            }

            return ( *this );
        }
    };

#endif

    namespace Json
    {
        const char* GetVersion()
        {
            return JSON4C4VERSION;
        }

        static const char* const StatusString[] = { "Ok",
                                                    "Could not open file",
                                                    "Could not read file",
                                                    "Invalid JSON text",
                                                    "Unterminated text buffer",
                                                    "Expected opening curly braces",
                                                    "Expected closing curly braces or comma",
                                                    "Expected opening square braces",
                                                    "Expected closing square braces or comma",
                                                    "Illegal character",
                                                    "Illegal escape",
                                                    "Expected colon",
                                                    "Expected begining double quotes",
                                                    "Expected ending double quotes",
                                                    "Invalid number",
                                                    "Expected 'true' or 'false'",
                                                    "Expected 'null'",
                                                    "Expected a JSON value (object, array, string, number, 'true', 'false', or 'null')",
                                                    "Expected end of file",
                                                    "File ends prematurely",
                                                    "Numbers cannot have leading zeros",
                                                    "JSON structure is invalid",
                                                    "An expected object value pair was not found",
                                                    "An exception was caught",
                                                    "JSON structured data contains invalid value type",
                                                    "Could not find requested name of name/value pair" };

        TERATHON_API String<256> StatusToString( const Status& status ) noexcept
        {
            return StatusString[ (unsigned int)( status ) ];
        }

        alignas( 64 ) const int8 hexadecimalCharValue[ 55 ] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11,
                                                                12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15 };
#ifndef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS
        alignas( 64 ) const double minusPower10[ 310 ] = {
            0.0,    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300, 1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292,
            1e-291, 1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282, 1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274,
            1e-273, 1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264, 1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256,
            1e-255, 1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246, 1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238,
            1e-237, 1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228, 1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220,
            1e-219, 1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210, 1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202,
            1e-201, 1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192, 1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184,
            1e-183, 1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174, 1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166,
            1e-165, 1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156, 1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148,
            1e-147, 1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138, 1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130,
            1e-129, 1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120, 1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112,
            1e-111, 1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102, 1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,
            1e-93,  1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,  1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,
            1e-75,  1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,  1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,
            1e-57,  1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,  1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,
            1e-39,  1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,  1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,
            1e-21,  1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,  1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,
            1e-3,   1e-2,   1e-1,   1.0 };

        alignas( 64 ) const double plusPower10[ 310 ] = { 1.0,   1e1,   1e2,   1e3,   1e4,   1e5,   1e6,   1e7,   1e8,   1e9,
                                                          1e10,  1e11,  1e12,  1e13,  1e14,  1e15,  1e16,  1e17,  1e18,  1e19,
                                                          1e20,  1e21,  1e22,  1e23,  1e24,  1e25,  1e26,  1e27,  1e28,  1e29,
                                                          1e30,  1e31,  1e32,  1e33,  1e34,  1e35,  1e36,  1e37,  1e38,  1e39,
                                                          1e40,  1e41,  1e42,  1e43,  1e44,  1e45,  1e46,  1e47,  1e48,  1e49,
                                                          1e50,  1e51,  1e52,  1e53,  1e54,  1e55,  1e56,  1e57,  1e58,  1e59,
                                                          1e60,  1e61,  1e62,  1e63,  1e64,  1e65,  1e66,  1e67,  1e68,  1e69,
                                                          1e70,  1e71,  1e72,  1e73,  1e74,  1e75,  1e76,  1e77,  1e78,  1e79,
                                                          1e80,  1e81,  1e82,  1e83,  1e84,  1e85,  1e86,  1e87,  1e88,  1e89,
                                                          1e90,  1e91,  1e92,  1e93,  1e94,  1e95,  1e96,  1e97,  1e98,  1e99,
                                                          1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
                                                          1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119,
                                                          1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129,
                                                          1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139,
                                                          1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149,
                                                          1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159,
                                                          1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169,
                                                          1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179,
                                                          1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189,
                                                          1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199,
                                                          1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209,
                                                          1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
                                                          1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229,
                                                          1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239,
                                                          1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249,
                                                          1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259,
                                                          1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269,
                                                          1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279,
                                                          1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289,
                                                          1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299,
                                                          1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308, __builtin_huge_val() };

#endif

        constexpr uint8 space          = 0x20;
        constexpr uint8 tab            = 0x09;
        constexpr uint8 newLine        = 0x0A;
        constexpr uint8 carriageReturn = 0x0D;

        class StringValue;
        class NumberValue;
        class BoolValue;
        class NullValue;
        class ArrayValue;

        void ExpandArray( Array<char>& array, const char c, const uint32 n )
        {
            for ( uint32 a = 0; a != n; a++ )
            {
                array.AppendArrayElement( c );
            }
        }

        int32 ComputeWhitespaceLength( const char* text ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            while ( byte[ 0 ] != 0 && ( byte[ 0 ] == space || byte[ 0 ] == tab || byte[ 0 ] == newLine || byte[ 0 ] == carriageReturn ) )
            {
                byte++;
            }

            return int32( reinterpret_cast<const char*>( byte ) - text );
        }

        int32 ComputeFloatLiteralLength( const char* text ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            while ( byte[ 0 ] != 0 && ( ( byte[ 0 ] >= '0' && byte[ 0 ] <= '9' ) || ( byte[ 0 ] == '.' ) || ( byte[ 0 ] == 'e' ) || ( byte[ 0 ] == 'E' ) ||
                                        ( byte[ 0 ] == '+' ) || ( byte[ 0 ] == '-' ) ) )
            {
                byte++;
            }

            return int32( reinterpret_cast<const char*>( byte ) - text );
        }

        int32 ReadEscapeChar( const char* text, uint32* value ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );
            uint32       c    = byte[ 0 ];

            if ( ( c == '\"' ) || ( c == '\\' ) || ( c == '/' ) )
            {
                *value = c;
                return ( 1 );
            }
            else if ( c == 'b' )
            {
                *value = '\b';
                return ( 1 );
            }
            else if ( c == 'f' )
            {
                *value = '\f';
                return ( 1 );
            }
            else if ( c == 'n' )
            {
                *value = '\n';
                return ( 1 );
            }
            else if ( c == 'r' )
            {
                *value = '\r';
                return ( 1 );
            }
            else if ( c == 't' )
            {
                *value = '\t';
                return ( 1 );
            }

            return ( 0 );
        }

        int32 ReadStringEscapeChar( const char* text, int32* stringLength, char* restrict string ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );
            uint32       c    = byte[ 0 ];

            if ( c == 'u' )
            {
                uint32 code = 0;

                for ( machine a = 1; a <= 4; a++ )
                {
                    c = byte[ a ] - '0';
                    if ( c >= 55U )
                    {
                        return ( 0 );
                    }

                    int32 x = hexadecimalCharValue[ c ];
                    if ( x < 0 )
                    {
                        return ( 0 );
                    }

                    code = ( code << 4 ) | x;
                }

                if ( code != 0 )
                {
                    if ( string )
                    {
                        *stringLength = Text::WriteUnicodeChar( string, code );
                    }
                    else
                    {
                        *stringLength = 1 + ( code >= 0x000080 ) + ( code >= 0x000800 );
                    }

                    return ( 5 );
                }
            }
            else
            {
                uint32 value;

                int32 textLength = ReadEscapeChar( text, &value );
                if ( textLength != 0 )
                {
                    if ( string )
                    {
                        *string = char( value );
                    }

                    *stringLength = 1;
                    return ( textLength );
                }
            }

            return ( 0 );
        }

        Status ReadStringLiteral( const char* text, int32* textLength, int32* stringLength, char* restrict string = nullptr ) noexcept( false )
        {
            const uint8* byte  = reinterpret_cast<const uint8*>( text );
            int32        count = 0;

            for ( ;; )
            {
                uint32 c = byte[ 0 ];
                if ( c == 0 )
                {
                    return Status::kPrematureNullTerminator;
                }

                if ( c == '\"' )
                {
                    break;
                }

                if ( ( c < 32U ) || ( c == 127U ) )
                {
                    return ( Status::kStringIllegalChar );
                }

                if ( c != '\\' )
                {
                    int32 len = Text::ValidateUnicodeChar( reinterpret_cast<const char*>( byte ) );
                    if ( len == 0 )
                    {
                        return ( Status::kStringIllegalChar );
                    }

                    if ( string )
                    {
                        for ( machine a = 0; a < len; a++ )
                        {
                            string[ a ] = char( byte[ a ] );
                        }

                        string += len;
                    }

                    byte += len;
                    count += len;
                }
                else
                {
                    int32 stringLen;

                    int32 textLen = ReadStringEscapeChar( reinterpret_cast<const char*>( ++byte ), &stringLen, string );
                    if ( textLen == 0 )
                    {
                        return ( Status::kStringIllegalEscape );
                    }

                    if ( string )
                    {
                        string += stringLen;
                    }

                    byte += textLen;
                    count += stringLen;
                }
            }

            *textLength   = int32( reinterpret_cast<const char*>( byte ) - text );
            *stringLength = count;

            return ( Status::kOk );
        }

        Status ReadDoubleLiteral( const char* text, int32* textLength, double* value )
        {
#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS
            *textLength = ComputeFloatLiteralLength( text );

            if ( *textLength == 0 )
            {
                return ( Status::kFloatInvalid );
            }

            if ( text[ 0 ] == '0' )
            {
                if ( *textLength > 1 && text[ 1 ] != '.' )
                {
                    return ( Status::kNumbersCannotHaveLeadingZeros );
                }
            }

            char* str = new char[ ( *textLength ) + 1 ];

#    ifdef _MSC_VER

            strncpy_s( str, ( *textLength ) + 1, text, ( *textLength ) );

#    else

            strncpy( str, text, ( *textLength ) );

#    endif

            str[ *textLength ] = 0;

            char*  end = nullptr;
            double v   = strtod( str, &end );

            if ( str == end )
            {
                delete[] str;
                return ( Status::kFloatInvalid );
            }

            delete[] str;

            *value = v;
            return ( Status::kOk );

#else
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            double v               = 0.0F;
            bool   digitFlag       = false;
            bool   wholeFlag       = false;
            bool   leadingZeroFlag = ( byte[ 0 ] == '0' );

            for ( ;; byte++ )
            {
                uint32 x = byte[ 0 ] - '0';
                if ( x < 10U )
                {
                    if ( digitFlag && leadingZeroFlag )
                    {
                        return ( Status::kNumbersCannotHaveLeadingZeros );
                    }

                    v         = v * 10.0F + double( x );
                    digitFlag = true;
                    wholeFlag = true;
                }
                else if ( x == 47 )
                {
                    if ( !digitFlag )
                    {
                        return ( Status::kFloatInvalid );
                    }

                    digitFlag = false;
                }
                else
                {
                    break;
                }
            }

            if ( wholeFlag && !digitFlag )
            {
                return ( Status::kFloatInvalid );
            }

            bool fractionFlag = false;

            uint32 c = byte[ 0 ];
            if ( c == '.' )
            {
                digitFlag      = false;
                double decimal = 10.0F;
                for ( ++byte;; byte++ )
                {
                    uint32 x = byte[ 0 ] - '0';
                    if ( x < 10U )
                    {
                        v += double( x ) / decimal;
                        digitFlag    = true;
                        fractionFlag = true;
                        decimal *= 10.0F;
                    }
                    else if ( x == 47 )
                    {
                        if ( !digitFlag )
                        {
                            return ( Status::kFloatInvalid );
                        }

                        digitFlag = false;
                    }
                    else
                    {
                        break;
                    }
                }

                if ( fractionFlag && !digitFlag )
                {
                    return ( Status::kFloatInvalid );
                }

                c = byte[ 0 ];
            }

            if ( !( wholeFlag || fractionFlag ) )
            {
                return ( Status::kFloatInvalid );
            }

            if ( ( c == 'e' ) || ( c == 'E' ) )
            {
                bool negative = false;

                c = ( ++byte )[ 0 ];
                if ( c == '-' )
                {
                    negative = true;
                    byte++;
                }
                else if ( c == '+' )
                {
                    byte++;
                }

                int32 exponent = 0;
                digitFlag      = false;
                for ( ;; byte++ )
                {
                    uint32 x = byte[ 0 ] - '0';
                    if ( x < 10U )
                    {
                        exponent  = Min( exponent * 10 + x, 65535 );
                        digitFlag = true;
                    }
                    else if ( x == 47 )
                    {
                        if ( !digitFlag )
                        {
                            return ( Status::kFloatInvalid );
                        }

                        digitFlag = false;
                    }
                    else
                    {
                        break;
                    }
                }

                if ( !digitFlag )
                {
                    return ( Status::kFloatInvalid );
                }

                if ( negative )
                {
                    v *= minusPower10[ MaxZero( 309 - exponent ) ];
                }
                else
                {
                    v *= plusPower10[ Min( exponent, 309 ) ];
                }
            }

            *value      = v;
            *textLength = int32( reinterpret_cast<const char*>( byte ) - text );

            return ( Status::kOk );
#endif
        }

        bool ParseSign( const char*& text ) noexcept
        {
            char c = text[ 0 ];

            if ( c == '-' )
            {
                text++;
                text += ComputeWhitespaceLength( text );
                return ( true );
            }

            if ( c == '+' )
            {
                text++;
                text += ComputeWhitespaceLength( text );
            }

            return ( false );
        }

        template <class T>
        Status ParseValue( Value*& value, const char*& text ) noexcept
        {
            T* jsonValue = nullptr;

            Status status = MayThrow(
                [ & ]()
                {
                    jsonValue = new T;
                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                return status;
            }

            status = MayThrow( [ & ]() { return T::Parse( jsonValue, text ); } );

            if ( status == Status::kOk )
            {
                value = jsonValue;

                return status;
            }

            delete jsonValue;

            return status;
        };

        Status ParseAnyValue( Value*& value, const char*& text ) noexcept
        {
            const char* start = text;

            Status status = ParseValue<StringValue>( value, text );
            if ( status == Status::kOk || status != Status::kExpectedBeginingDoubleQuotes )
            {
                return status;
            }
            text = start;

            status = ParseValue<NumberValue>( value, text );
            if ( status == Status::kOk || status == Status::kNumbersCannotHaveLeadingZeros )
            {
                return status;
            }
            text = start;

            if ( ParseValue<BoolValue>( value, text ) == Status::kOk )
            {
                return Status::kOk;
            }
            text = start;

            if ( ParseValue<NullValue>( value, text ) == Status::kOk )
            {
                return Status::kOk;
            }
            text = start;

            status = ParseValue<ArrayValue>( value, text );
            if ( status == Status::kOk || status != Status::kExpectedOpeningSquareBrackets )
            {
                return status;
            }

            status = ParseValue<ObjectValue>( value, text );
            if ( status == Status::kOk || status != Status::kExpectedOpeningCurlyBraces || status == Status::kNumbersCannotHaveLeadingZeros )
            {
                return status;
            }

            return Status::kExpectedJsonValue;
        }

        class StringValue final : public Value
        {
        public:
            static Status Parse( StringValue* jsonStringValue, const char*& text ) noexcept
            {
                if ( text[ 0 ] != '"' )
                {
                    return ( Status::kExpectedBeginingDoubleQuotes );
                }

                int32 textLength;
                int32 stringLength;

                text++;

                Status status = ReadStringLiteral( text, &textLength, &stringLength );
                if ( status != Status::kOk )
                {
                    return ( status );
                }

                if ( jsonStringValue )
                {
                    status = MayThrow(
                        [ & ]()
                        {
                            jsonStringValue->data.SetStringLength( stringLength );
                            return Status::kOk;
                        } );

                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    ReadStringLiteral( text, &textLength, &stringLength, jsonStringValue->data );
                }

                text += textLength;

                if ( text[ 0 ] != '"' )
                {
                    return ( Status::kExpectedEndingDoubleQuotes );
                }

                text++;

                return ( Status::kOk );
            }

        public:
            using DataType = String<>;

            DataType data;
            ~StringValue() noexcept override {};

            String<>* GetStringData() noexcept override
            {
                return &data;
            }

            const String<>* GetStringData() const noexcept override
            {
                return &data;
            }

            Status Write( File& file, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {
                file << '\"' << data << '\"';

                return Status::kOk;
            }
        };

        class NumberValue final : public Value
        {
        public:
            static Status Parse( NumberValue* value, const char*& text ) noexcept
            {
                int32  length;
                double doubleData;

                bool negative = ParseSign( text );

                Status status = MayThrow( [ & ]() { return ReadDoubleLiteral( text, &length, &doubleData ); } );

                if ( status != Status::kOk )
                {
                    return ( status );
                }

                if ( negative )
                {
                    doubleData = -doubleData;
                }

                if ( value )
                {
                    value->data = doubleData;
                }

                text += length;
                text += ComputeWhitespaceLength( text );

                return ( Status::kOk );
            }

        public:
            using DataType = double;
            DataType data  = 0;

            ~NumberValue() override {};

            double* GetDoubleData() noexcept override
            {
                return &data;
            }

            const double* GetDoubleData() const noexcept override
            {
                return &data;
            }

            Status Write( File& file, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {

#ifdef JSON4C4_USE_SYSTEM_DOUBLE_STRING_CONVERSIONS
                constexpr int maxDoubleDigits = 32;
                char          output[ maxDoubleDigits ];

                snprintf( output, maxDoubleDigits, "%.17g", data );

                file << output;
#else
                file << Text::FloatToString( data );
#endif

                return Status::kOk;
            }
        };

        class BoolValue final : public Value
        {
        public:
            static Status Parse( BoolValue* value, const char*& text ) noexcept
            {
                if ( text[ 0 ] == 't' && text[ 1 ] == 'r' && text[ 2 ] == 'u' && text[ 3 ] == 'e' )
                {
                    value->data = true;
                    text += 4;

                    return Status::kOk;
                }

                if ( text[ 0 ] == 'f' && text[ 1 ] == 'a' && text[ 2 ] == 'l' && text[ 3 ] == 's' && text[ 4 ] == 'e' )
                {
                    value->data = false;
                    text += 5;

                    return Status::kOk;
                }

                return Status::kExpectedTrueOrFalse;
            }

        public:
            using DataType = bool;
            DataType data  = true;

            ~BoolValue() override {};

            bool* GetBoolData() noexcept override
            {
                return &data;
            }

            const bool* GetBoolData() const noexcept override
            {
                return &data;
            }

            Status Write( File& file, uint32, const char, Array<char>& ) const override
            {

                if ( data )
                {
                    file << "true";
                }
                else
                {
                    file << "false";
                }

                return Status::kOk;
            }
        };

        class NullValue final : public Value
        {
        public:
            static Status Parse( NullValue* /*JsonNullValue*/, const char*& text ) noexcept
            {
                if ( text[ 0 ] == 'n' && text[ 1 ] == 'u' && text[ 2 ] == 'l' && text[ 3 ] == 'l' )
                {
                    text += 4;

                    return Status::kOk;
                }

                return Status::kExpectedNull;
            }

        public:
            using DataType = Null;
            DataType data;

            ~NullValue() override {};

            Null* GetJsonNullData() noexcept override
            {
                return &data;
            }

            const Null* GetJsonNullData() const noexcept override
            {
                return &data;
            }

            Status Write( File& file, uint32 /*indentationLength*/, const char /*indentationChar*/, Array<char>& /*indentationCharArray*/ ) const override
            {

                file << "null";

                return Status::kOk;
            }
        };

        namespace Detail
        {
            template <>
            Value* CreateValueFor( const String<>& ) noexcept( false )
            {
                return new StringValue;
            }

            template <>
            Value* CreateValueFor( const double& ) noexcept( false )
            {
                return new NumberValue;
            }

            template <>
            Value* CreateValueFor( const bool& ) noexcept( false )
            {
                return new BoolValue;
            }

            template <>
            Value* CreateValueFor( const Null& ) noexcept( false )
            {
                return new NullValue;
            }

        } // namespace Detail

        Status ArrayValue::Parse( ArrayValue* jsonArray, const char*& text ) noexcept
        {
            if ( text[ 0 ] != '[' )
            {
                return Status::kExpectedOpeningSquareBrackets;
            }

            bool emptyArray = true;

            for ( ;; )
            {
                text++;
                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] == ']' )
                {
                    if ( !emptyArray )
                    {
                        return Status::kExpectedJsonValue;
                    }

                    text++;

                    return Status::kOk;
                }

                Value* jsonValue = nullptr;
                Status status    = ParseAnyValue( jsonValue, text );
                if ( status != Status::kOk )
                {
                    return status;
                }

                status = MayThrow(
                    [ & ]()
                    {
                        jsonArray->data.AppendArrayElement( jsonValue );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    return status;
                }

                emptyArray = false;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ',' )
                {
                    break;
                }
            }

            if ( text[ 0 ] != ']' )
            {
                return Status::kExpectedClosingSquareBracketsOrComma;
            }

            text++;

            return Status::kOk;
        }

        ArrayValue::~ArrayValue() {};

        Array<Value*>* ArrayValue::GetJsonValueArrayData() noexcept
        {
            return &data;
        }

        const Array<Value*>* ArrayValue::GetJsonValueArrayData() const noexcept
        {
            return &data;
        }

        Status ArrayValue::Write( File& file, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const
        {
            file << "[\n";

            ExpandArray( indentationCharArray, indentationChar, indentationLength );
            for ( int32 a = 0; a != data.GetArrayElementCount(); a++ )
            {
                const Value* value = data[ a ];

                file.WriteFile( indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

                Status status = value->Write( file, indentationLength, indentationChar, indentationCharArray );
                if ( status != Status::kOk )
                {
                    return status;
                }

                file << ( a < data.GetArrayElementCount() - 1 ? "," : "" ) << '\n';
            }

            indentationCharArray.SetArrayElementCount( indentationCharArray.GetArrayElementCount() - indentationLength );

            file.WriteFile( indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );
            file << "]";

            return Status::kOk;
        }

        Value::~Value() noexcept
        {
        }

        bool ObjectValue::InsertAccountedMapElement( Value* element )
        {
            insertionOrder.AppendArrayElement( element->name );

            return this->InsertMapElement( element );
        }

        Status ObjectValue::Parse( ObjectValue* jsonObject, const char*& text ) noexcept
        {
            if ( text[ 0 ] != '{' )
            {
                return Status::kExpectedOpeningCurlyBraces;
            }

            bool emptyObject = true;

            for ( ;; )
            {
                text++;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] == '}' )
                {
                    if ( !emptyObject )
                    {
                        return Status::kExpectedJsonValue;
                    }
                    text++;

                    return Status::kOk;
                }

                StringValue name;
                Status      status = StringValue::Parse( &name, text );
                if ( status != Status::kOk )
                {
                    return status;
                }

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ':' )
                {
                    return Status::kExpectedColon;
                }

                text++;
                text += ComputeWhitespaceLength( text );

                Value* jsonValue = nullptr;

                status = ParseAnyValue( jsonValue, text );
                if ( status != Status::kOk )
                {
                    return status;
                }

                status = MayThrow(
                    [ & ]()
                    {
                        jsonValue->name = name.data;
                        jsonObject->InsertAccountedMapElement( jsonValue );

                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    return status;
                }

                emptyObject = false;

                text += ComputeWhitespaceLength( text );

                if ( text[ 0 ] != ',' )
                {
                    break;
                }
            }

            if ( text[ 0 ] != '}' )
            {
                return Status::kExpectedClosingCurlyBracesOrComma;
            }

            text++;

            return Status::kOk;
        }

        ObjectValue::~ObjectValue() noexcept
        {
        }

        ObjectValue* ObjectValue::AsJsonObjectValue() noexcept
        {
            return this;
        }

        const ObjectValue* ObjectValue::AsJsonObjectValue() const noexcept
        {
            return this;
        }

        const String<>* ObjectValue::FindString( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const String<>* data = jsonValue->GetDataAsPointerTo<String<>>() )
                {
                    return data;
                }
            }

            return nullptr;
        }

        const double* ObjectValue::FindNumber( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const double* data = jsonValue->GetDataAsPointerTo<double>() )
                {
                    return data;
                }
            }

            return nullptr;
        }

        const bool* ObjectValue::FindBool( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const bool* data = jsonValue->GetDataAsPointerTo<bool>() )
                {
                    return data;
                }
            }

            return nullptr;
        }

        const Null* ObjectValue::FindJsonNull( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const Null* data = jsonValue->GetDataAsPointerTo<Null>() )
                {
                    return data;
                }
            }

            return nullptr;
        }

        const Array<Value*>* ObjectValue::FindJsonValueArray( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const Array<Value*>* data = jsonValue->GetDataAsPointerTo<Array<Value*>>() )
                {
                    return data;
                }
            }

            return nullptr;
        }

        const ObjectValue* ObjectValue::FindJsonObjectValue( const char* name ) const noexcept
        {
            if ( Value* jsonValue = FindMapElement( name ) )
            {
                if ( const ObjectValue* value = jsonValue->AsJsonObjectValue() )
                {
                    return value;
                }
            }

            return nullptr;
        }

        Status ObjectValue::Write( File& file, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const
        {
            file << "{\n";

            ExpandArray( indentationCharArray, indentationChar, indentationLength );

            for ( int32 a = 0; a != insertionOrder.GetArrayElementCount(); a++ )
            {

                Value* value = this->FindMapElement( insertionOrder[ a ] );

                if ( !value )
                {
                    return Status::kMissingObjectElement;
                }

                file.WriteFile( indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );

                file << '"' << value->name << "\" : ";

                Status status = value->Write( file, indentationLength, indentationChar, indentationCharArray );
                if ( status != Status::kOk )
                {
                    return status;
                }

                file << ( a < insertionOrder.GetArrayElementCount() - 1 ? "," : "" ) << '\n';
            }

            indentationCharArray.SetArrayElementCount( indentationCharArray.GetArrayElementCount() - indentationLength );

            file.WriteFile( indentationCharArray.begin(), indentationCharArray.GetArrayElementCount() );
            file << "}";

            return Status::kOk;
        }

        Status ParseJsonRoot( Value*& jsonRoot, const Array<char>& nullTerminatedCharArray, int32* errorLine, int32* errorColumn ) noexcept
        {
            if ( nullTerminatedCharArray.GetArrayElementCount() < 3 )
            {
                return Status::kTextBufferContentsInvalid;
            }

            if ( *( nullTerminatedCharArray.end() - 1 ) != 0 )
            {
                return Status::kUnterminatedTextBuffer;
            }

            const char* text = &nullTerminatedCharArray[ 0 ];
            text += ComputeWhitespaceLength( text );

            const char* start = text;

            Status status = ParseAnyValue( jsonRoot, text );

            text += ComputeWhitespaceLength( text );
            if ( status == Status::kOk && text[ 0 ] != 0 )
            {
                status = Status::kExpectedEndOfFile;
            }

            if ( status != Status::kOk )
            {
                *errorLine   = 1;
                *errorColumn = 1;

                while ( text != start )
                {
                    if ( ( --text )[ 0 ] == '\n' )
                    {
                        ( *errorLine )++;
                    }
                    *errorColumn += ( ( *errorLine ) == 1 ) ? 1 : 0;
                }
            }

            return status;
        }

        StructuredData::~StructuredData() noexcept
        {
            if ( rootJsonValue )
            {
                delete rootJsonValue;
            }
        }

        ParseResult StructuredData::Parse( const char* fileName ) noexcept
        {

            File file;

            if ( file.OpenFile( fileName, kFileReadOnly ) != kFileOkay )
            {
                return ParseResult { Status::KFileOpenError, 0, 0 };
            }

            Terathon::uint64 fileSize = file.GetFileSize();

            Array<char> nullTerminatedText;

            Status status = MayThrow(
                [ & ]()
                {
                    nullTerminatedText.SetArrayElementCount( fileSize + 1, 0 );

                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                return ParseResult { status, 0, 0 };
            }

            if ( file.ReadFile( nullTerminatedText.begin(), fileSize ) != kFileOkay )
            {
                return ParseResult { Status::KFileReadError, 0, 0 };
            }

            file.CloseFile();

            return Parse( nullTerminatedText );
        }

        ParseResult StructuredData::Parse( const Array<char>& nullTerminatedTextBuffer ) noexcept
        {
            if ( nullTerminatedTextBuffer.GetArrayElementCount() < 3 )
            {
                return ParseResult { Status::kTextBufferContentsInvalid, 0, 0 };
            }

            if ( *( nullTerminatedTextBuffer.end() - 1 ) != 0 )
            {
                return ParseResult { Status::kUnterminatedTextBuffer, 0, 0 };
            }

            if ( rootJsonValue )
            {
                delete rootJsonValue;
                rootJsonValue = nullptr;
            }

            ParseResult parseResult;
            parseResult.status = ParseJsonRoot( rootJsonValue, nullTerminatedTextBuffer, &parseResult.errorLine, &parseResult.errorColumn );

            return parseResult;
        }

        TERATHON_API Status StructuredData::Write( const char* fileName, const uint32 indentationLength, const char indentationChar ) noexcept
        {
            File file;
            if ( file.OpenFile( fileName, kFileCreate ) != kFileOkay )
            {
                return Status::KFileOpenError;
            }

            Array<char> indentCharArray;

            if ( rootJsonValue )
            {
                Status status = rootJsonValue->Write( file, indentationLength, indentationChar, indentCharArray );

                file.CloseFile();

                return status;
            }

            return Status::kInvalidStructuredData;
        }

        const Value* StructuredData::GetRootJsonValue() const noexcept
        {
            return rootJsonValue;
        }

        Value* StructuredData::GetRootJsonValue() noexcept
        {
            return rootJsonValue;
        }

        String<256> ParseResultToString( const ParseResult& parseResult ) noexcept
        {
            String<256> string = StatusToString( parseResult.status );

            if ( parseResult.errorLine != 0 && parseResult.status != Status::kException )
            {
                string += String<24>( ". Line: " ) + String<24>( parseResult.errorLine ) + String<24>( ", column: " ) + String<24>( parseResult.errorColumn );
            }

            return string;
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        namespace Detail
        {
            template <>
            Value* CreateValueFor( const std::string& )
            {
                return new StringValue;
            }
        } // namespace Detail

        template <>
        TERATHON_API Status Validate( const Value* value, const std::string& /*data*/ ) noexcept
        {
            const String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return Status::kOk;
        }

        template <>
        TERATHON_API Status Deserialize( const Value* value, std::string& data ) noexcept
        {
            const String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    data = static_cast<const char*>( *valueData );

                    return Status::kOk;
                } );
        }

        template <>
        TERATHON_API Status Serialize( Value* value, const std::string& data ) noexcept
        {
            String<>* valueData = value->GetDataAsPointerTo<String<>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    *valueData = data.c_str();

                    return Status::kOk;
                } );
        }

#endif

    } // namespace Json
} // namespace C4

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::String<>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::String<>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::String<>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, double& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const double& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const double& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, bool& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const bool& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const bool& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::Null& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::Null& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::Null& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;

template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;
template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;

#endif
