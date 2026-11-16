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

// RFC 8259 (STD90) Compliant JSON library for the C4 Engine

#ifndef C4Json_h
#define C4Json_h

#if defined( _MSC_VER )

#    pragma warning( push )

#endif

#include "TSArray.h"
#include "TSMap.h"
#include "TSString.h"

#if defined( _MSC_VER )

#    pragma warning( pop )
#    pragma warning( push )
#    pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#endif

#ifndef JSON4C4_MAX_NESTING_DEPTH

#    define JSON4C4_MAX_NESTING_DEPTH 256

#endif

#ifndef JSON4C4_MAX_VALUE_COUNT

#    define JSON4C4_MAX_VALUE_COUNT 0x7FFFFFFFFFFFFFFF

#endif

#ifndef JSON4C4_MAX_STRING_BYTE_COUNT

#    define JSON4C4_MAX_STRING_BYTE_COUNT 0x7FFFFFFFFFFFFFFF

#endif

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    error "JSON4C4_ENABLE_STD_SUPPORT_INTERNAL appears defined. Use JSON4C4_DISABLE_STD_SUPPORT instead to control Json4C4 behavior."

#endif

#ifndef TERATHON_NO_SYSTEM

#    ifndef JSON4C4_DISABLE_STD_SUPPORT

#        define JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    endif

#endif

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

#    include <map>
#    include <string>
#    include <vector>

#endif

namespace C4
{
    namespace Json
    {
        using namespace Terathon;

        namespace Detail
        {
            template <typename T>
            struct Identity
            {
                using Type = T;
            };

            template <typename T>
            T&& Forward( typename Identity<T>::Type&& arg )
            {
                return static_cast<typename Identity<T>::Type&&>( arg );
            }

            template <class T, T v>
            struct IntegralConstant
            {
                using Type                       = IntegralConstant;
                using ValueType                  = T;
                static constexpr ValueType Value = v;

                constexpr operator ValueType() const noexcept
                {
                    return Value;
                }

                constexpr ValueType operator()() const noexcept
                {
                    return Value;
                }
            };

            using TrueType  = IntegralConstant<bool, true>;
            using FalseType = IntegralConstant<bool, false>;

            template <class... Args>
            using VoidType = void;

            template <class T, class U>
            struct IsSame : FalseType
            {
            };

            template <class T>
            struct IsSame<T, T> : TrueType
            {
            };

            struct Optional
            {
            };

        } // namespace Detail

        enum class Status : unsigned int
        {
            kOk = 0,
            kFileOpenError,
            kFileReadError,
            kFileTooLarge,
            kTextBufferContentsInvalid,
            kUnterminatedTextBuffer,
            kExpectedOpeningCurlyBraces,
            kExpectedClosingCurlyBracesOrComma,
            kExpectedOpeningSquareBrackets,
            kExpectedClosingSquareBracketsOrComma,
            kStringIllegalChar,
            kStringIllegalEscape,
            kExpectedColon,
            kExpectedBeginningDoubleQuotes,
            kExpectedEndingDoubleQuotes,
            kFloatInvalid,
            kExpectedTrueOrFalse,
            kExpectedNull,
            kExpectedJsonValue,
            kExpectedEndOfFile,
            kPrematureNullTerminator,
            kNumbersCannotHaveLeadingZeros,
            kInvalidStructuredData,
            kException,
            kInvalidValueType,
            kNameNotPresent,
            kMaximumNestingDepthExceeded,
            kDuplicateName,
            kFileWriteError,
            kMaximumValueCountExceeded,
            kMaximumStringByteCountExceeded,
            kAllocationRefused,
            kStringHoldsZero,
            kIndexOutOfRange
        };

        TERATHON_API String<256> StatusToString( const Status& parseResult ) noexcept;

        struct ParseResult
        {
            Status status      = Status::kOk;
            int32  errorLine   = 0;
            int32  errorColumn = 0;
        };

        enum class DuplicateNames : unsigned int
        {
            kReject = 0,
            kKeepFirst,
            kKeepLast
        };

        struct ParseOptions
        {
            DuplicateNames duplicateNames     = DuplicateNames::kReject;
            int32          maxNestingDepth    = JSON4C4_MAX_NESTING_DEPTH;
            int64          maxValueCount      = JSON4C4_MAX_VALUE_COUNT;
            int64          maxStringByteCount = JSON4C4_MAX_STRING_BYTE_COUNT;
        };

        inline const Detail::Optional optional;

        namespace Detail
        {
            template <class Func>
            Status MayThrow( Func&& func ) noexcept
            {
                // When TERATHON_NO_SYSTEM is defined, operator new doesn't throw, so there is no much point in including try/catch
#if ( ( ( defined( __GNUC__ ) || defined( __GNUG__ ) || defined( __clang__ ) ) && defined( __cpp_exceptions ) ) ||                                             \
      ( defined( _MSC_VER ) && defined( _CPPUNWIND ) ) ) &&                                                                                                    \
    !defined( TERATHON_NO_SYSTEM )
                try
#endif
                {
                    return func();
                }
#if ( ( ( defined( __GNUC__ ) || defined( __GNUG__ ) || defined( __clang__ ) ) && defined( __cpp_exceptions ) ) ||                                             \
      ( defined( _MSC_VER ) && defined( _CPPUNWIND ) ) ) &&                                                                                                    \
    !defined( TERATHON_NO_SYSTEM )
                catch ( ... )
                {
                }
                return Status::kException;
#endif
            }
        } // namespace Detail

        TERATHON_API const char* GetVersion();

        // Basic Types :
        // String<>
        // double
        // bool
        // and:
        struct Null
        {
        };

        template <class ValueType>
        class ObjectMapElement : public Terathon::MapElement<ObjectMapElement<ValueType>>
        {
        public:
            using KeyType = Terathon::String<>;

            KeyType   name;
            ValueType data = ValueType();

            TERATHON_API inline const KeyType& GetKey() const noexcept
            {
                return name;
            };
        };

        template <class ValueType>
        class ObjectMap : public Terathon::Map<ObjectMapElement<ValueType>>
        {
        public:
            using ElementType = ObjectMapElement<ValueType>;
        };

        namespace Detail
        {
            template <class T>
            struct TypeIdentity
            {
                using Type = T;
            };

            template <class T>
            TypeIdentity<T&&> TryAddRValueReference( int );

            template <class T>
            TypeIdentity<T> TryAddRValueReference( ... );

            template <class T>
            struct AddRValueReference : decltype( TryAddRValueReference<T>( 0 ) )
            {
            };

            template <typename T>
            typename AddRValueReference<T>::Type DeclVal() noexcept
            {
                constexpr bool AlwaysFalse = !Detail::IsSame<T, T>::Value;

                static_assert( AlwaysFalse, "DeclVal not allowed in an evaluated context" );
            }

            struct NoneSuch
            {
                ~NoneSuch()                       = delete;
                NoneSuch( const NoneSuch& )       = delete;
                void operator=( const NoneSuch& ) = delete;
            };

            template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
            struct Detector
            {
                using ValueType = FalseType;
                using Type      = Default;
            };

            template <class Default, template <class...> class Op, class... Args>
            struct Detector<Default, VoidType<Op<Args...>>, Op, Args...>
            {
                using ValueType = TrueType;
                using Type      = Op<Args...>;
            };

            template <template <class...> class Op, class... Args>
            using DetectedType = typename Detector<NoneSuch, void, Op, Args...>::Type;

            template <class Expected, template <class...> class Op, class... Args>
            using IsDetectedExact = IsSame<Expected, DetectedType<Op, Args...>>;

        } // namespace Detail

        TERATHON_API String<256> ParseResultToString( const ParseResult& parseResult ) noexcept;

        // Gives a document its memory. AllocateBlock returns size bytes aligned to 8 bytes, or nullptr to refuse
        // them, and FreeBlock takes a block back with the size it was allocated with. Sizes are multiples of 8. A
        // refused block that the document only wanted to grow is asked for again at smaller sizes, down to what the
        // request needs. A source is called only from the thread that uses the document, and it must outlive the
        // document.
        class BlockSource
        {
        public:
            virtual void* AllocateBlock( uint64 size ) noexcept          = 0;
            virtual void  FreeBlock( void* block, uint64 size ) noexcept = 0;

        protected:
            ~BlockSource() = default;
        };

        // Allocates blocks with operator new.
        TERATHON_API BlockSource& GetDefaultBlockSource() noexcept;

        enum class Type : uint8
        {
            kNull,
            kBool,
            kNumber,
            kString,
            kArray,
            kObject
        };

        // Bytes and their count, without a terminating zero. A string may hold zeros.
        class StringView
        {
        public:
            StringView() = default;

            StringView( const char* stringText, uint32 stringLength ) noexcept : text( stringText ), length( stringLength )
            {
            }

            explicit operator bool() const noexcept
            {
                return text != nullptr;
            }

            const char* GetText() const noexcept
            {
                return text;
            }

            uint32 GetLength() const noexcept
            {
                return length;
            }

            const char* begin() const noexcept
            {
                return text;
            }

            const char* end() const noexcept
            {
                return text + length;
            }

            TERATHON_API bool operator==( const StringView& other ) const noexcept;
            TERATHON_API bool operator==( const char* other ) const noexcept;

            bool operator!=( const StringView& other ) const noexcept
            {
                return !( *this == other );
            }

            bool operator!=( const char* other ) const noexcept
            {
                return !( *this == other );
            }

        private:
            const char* text   = nullptr;
            uint32      length = 0;
        };

        class ArrayView;
        class ObjectView;
        class Member;
        class Node;
        class Builder;
        class StructuredData;

        class Value
        {
            friend class ArrayView;
            friend class ObjectView;
            friend class DocumentParser;
            friend class PendingValues;
            friend class Builder;
            friend class Node;
            friend class StructuredData;

        public:
            Type GetType() const noexcept
            {
                return type;
            }

            bool IsNull() const noexcept
            {
                return type == Type::kNull;
            }

            // A number written without a fraction or an exponent.
            bool IsInteger() const noexcept
            {
                return ( type == Type::kNumber ) && integer;
            }

            const bool* AsBool() const noexcept
            {
                return ( type == Type::kBool ) ? &data.boolean : nullptr;
            }

            const double* AsNumber() const noexcept
            {
                return ( type == Type::kNumber ) ? &data.number : nullptr;
            }

            StringView AsString() const noexcept
            {
                return ( type == Type::kString ) ? StringView( data.text, count ) : StringView();
            }

            inline ArrayView  AsArray() const noexcept;
            inline ObjectView AsObject() const noexcept;

        private:
            union Data
            {
                double        number;
                bool          boolean;
                const char*   text;
                const Value*  elements;
                const Member* members;
            };

            Type   type          = Type::kNull;
            bool   integer       = false;
            uint8  capacityShift = 0; // an edited array or object holds room for 1 << capacityShift
            uint32 count         = 0;
            Data   data          = {};

            uint32 GetCapacity() const noexcept
            {
                return ( capacityShift != 0 ) ? uint32( 1 ) << capacityShift : count;
            }
        };

        static_assert( sizeof( Value ) == 16, "A Value is 16 bytes" );

        class Member
        {
            friend class ObjectView;
            friend class DocumentParser;
            friend class PendingValues;
            friend class Builder;
            friend class Node;
            friend class StructuredData;

        public:
            const StringView& GetName() const noexcept
            {
                return name;
            }

            const Value& GetValue() const noexcept
            {
                return value;
            }

        private:
            StringView name;
            Value      value;
        };

        class ArrayView
        {
            friend class Value;

        public:
            ArrayView() = default;

            explicit operator bool() const noexcept
            {
                return array != nullptr;
            }

            uint32 GetElementCount() const noexcept
            {
                return array ? array->count : 0;
            }

            const Value* begin() const noexcept
            {
                return array ? array->data.elements : nullptr;
            }

            const Value* end() const noexcept
            {
                return begin() + GetElementCount();
            }

            const Value& operator[]( uint32 index ) const noexcept
            {
                return begin()[ index ];
            }

        private:
            const Value* array = nullptr;

            explicit ArrayView( const Value* arrayValue ) noexcept : array( arrayValue )
            {
            }
        };

        // Objects of more than 16 members find a name through an index of their members sorted by name, which
        // follows the room for their members. Smaller ones compare every name.
        class ObjectView
        {
            friend class Value;

        public:
            ObjectView() = default;

            explicit operator bool() const noexcept
            {
                return object != nullptr;
            }

            uint32 GetMemberCount() const noexcept
            {
                return object ? object->count : 0;
            }

            const Member* begin() const noexcept
            {
                return object ? object->data.members : nullptr;
            }

            const Member* end() const noexcept
            {
                return begin() + GetMemberCount();
            }

            TERATHON_API const Value* Find( const char* name, uint32 length ) const noexcept;
            TERATHON_API const Value* Find( const char* name ) const noexcept;

            const Value* Find( const StringView& name ) const noexcept
            {
                return Find( name.GetText(), name.GetLength() );
            }

            const bool* FindBool( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value ? value->AsBool() : nullptr;
            }

            const double* FindNumber( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value ? value->AsNumber() : nullptr;
            }

            StringView FindString( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value ? value->AsString() : StringView();
            }

            ArrayView FindArray( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value ? value->AsArray() : ArrayView();
            }

            ObjectView FindObject( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value ? value->AsObject() : ObjectView();
            }

            bool FindNull( const char* name ) const noexcept
            {
                const Value* value = Find( name );
                return value && value->IsNull();
            }

        private:
            const Value* object = nullptr;

            explicit ObjectView( const Value* objectValue ) noexcept : object( objectValue )
            {
            }
        };

        inline ArrayView Value::AsArray() const noexcept
        {
            return ( type == Type::kArray ) ? ArrayView( this ) : ArrayView();
        }

        inline ObjectView Value::AsObject() const noexcept
        {
            return ( type == Type::kObject ) ? ObjectView( this ) : ObjectView();
        }

        namespace Detail
        {
            inline uint32 GetTextLength( const char* text ) noexcept
            {
                uint32 length = 0;

                while ( text[ length ] != 0 )
                {
                    length++;
                }

                return length;
            }
        } // namespace Detail

        // A document. Everything it holds lives in blocks from its block source, which it hands back when it is
        // cleared, parsed again or destroyed. Values, strings and views taken from it are valid until then, except
        // that an edit moves the elements or members of the array or object it changes: pointers to them, and views
        // and pointers into them, are not valid after it, while views of the edited value itself stay valid. A
        // removal moves the later members down in place, so a pointer kept to one of them then points at another,
        // and an edit through it changes that one; find a value again after an edit.
        class StructuredData
        {
            friend class DocumentParser;
            friend class PendingValues;
            friend class Builder;
            friend class Node;

        public:
            TERATHON_API explicit StructuredData( BlockSource& source = GetDefaultBlockSource() ) noexcept;
            TERATHON_API ~StructuredData() noexcept;

            StructuredData( const StructuredData& )            = delete;
            StructuredData& operator=( const StructuredData& ) = delete;

            TERATHON_API ParseResult Parse( const char* fileName, const ParseOptions& options = ParseOptions() ) noexcept;
            TERATHON_API ParseResult Parse( const Array<char>& nullTerminatedTextBuffer, const ParseOptions& options = ParseOptions() ) noexcept;
            TERATHON_API ParseResult ParseText( const char* text, uint64 length, const ParseOptions& options = ParseOptions() ) noexcept;

            TERATHON_API Status Write( Array<char>& output, uint32 indentationLength = 2, char indentationChar = ' ' ) const noexcept;
            TERATHON_API Status Write( const char* fileName, uint32 indentationLength = 2, char indentationChar = ' ' ) const noexcept;

            const Value* GetRoot() const noexcept
            {
                return root;
            }

            template <class T>
            Status DeserializeTo( T& data ) const noexcept;

            // Replaces the document with the values data serializes to. A failure leaves the document empty.
            template <class T>
            Status SerializeFrom( const T& data ) noexcept;

            // Edits of a value of this document, taken from its views. The new value is built before anything
            // changes, so it may be built from the value it replaces or joins, and a failure leaves the document as
            // it was. Memory a removal frees, or an array or object outgrows, comes back when the document is cleared.
            template <class T>
            Status SerializeAt( const Value& target, const T& data ) noexcept;

            template <class T>
            Status AppendElement( const Value& array, const T& data ) noexcept;

            template <class T>
            Status AddMember( const Value& object, const char* name, uint32 length, const T& data ) noexcept;

            template <class T>
            Status AddMember( const Value& object, const char* name, const T& data ) noexcept
            {
                return AddMember( object, name, Detail::GetTextLength( name ), data );
            }

            TERATHON_API Status RemoveElement( const Value& array, uint32 index ) noexcept;
            TERATHON_API Status RemoveMember( const Value& object, const char* name, uint32 length ) noexcept;

            Status RemoveMember( const Value& object, const char* name ) noexcept
            {
                return RemoveMember( object, name, Detail::GetTextLength( name ) );
            }

            TERATHON_API Status EmptyObject( const Value& object ) noexcept;

            TERATHON_API void Clear() noexcept;

        private:
            struct Block
            {
                Block* next;
                uint64 size;
            };

            BlockSource* blockSource;
            Block*       blocks        = nullptr;
            char*        blockTop      = nullptr;
            char*        blockEnd      = nullptr;
            uint64       nextBlockSize = 0;
            const Value* root          = nullptr;
            Builder*     builder       = nullptr;

            enum class EditKind : uint8
            {
                kDocument,
                kReplace,
                kAppend,
                kAddMember
            };

            struct Edit
            {
                EditKind     kind;
                const Value* target;
                const char*  name;
                uint32       length;
            };

            void* AllocateAligned( uint64 size ) noexcept;
            char* AllocateBytes( uint64 size ) noexcept;
            void* AllocateInNewBlock( uint64 size, bool aligned ) noexcept;

            template <class T>
            Status Build( const Edit& edit, const T& data ) noexcept;

            TERATHON_API Status BeginBuild( const Edit& edit, Node& rootNode ) noexcept;
            TERATHON_API Status EndBuild( const Edit& edit, const Node& rootNode, Status status ) noexcept;

            Status CheckEdit( const Value& target, Type type ) const noexcept;
            Status Splice( const Edit& edit, const Value& built ) noexcept;
            bool   MakeRoom( Value& container ) noexcept;
        };

        // The names and array indices from the root to the value at fault when a read fails, innermost first. The
        // traced functions add to it as they return a failure.
        class ReadPath
        {
        public:
            struct Segment
            {
                const char* name; // nullptr for an array index
                uint32      length;
                uint32      index;
                bool        missing; // a member the object does not have
            };

            void AddName( const char* name, uint32 length, bool missing = false ) noexcept
            {
                Add( Segment { name, length, 0, missing } );
            }

            void AddName( const char* name, bool missing = false ) noexcept
            {
                uint32 length = 0;

                while ( name[ length ] != 0 )
                {
                    length++;
                }

                AddName( name, length, missing );
            }

            void AddIndex( uint32 index ) noexcept
            {
                Add( Segment { nullptr, 0, index, false } );
            }

            int32 GetSegmentCount() const noexcept
            {
                return segments.GetArrayElementCount();
            }

            const Segment& GetSegment( int32 index ) const noexcept
            {
                return segments[ index ];
            }

        private:
            Array<Segment> segments;

            void Add( const Segment& segment ) noexcept
            {
                Detail::MayThrow(
                    [ & ]()
                    {
                        segments.AppendArrayElement( segment );
                        return Status::kOk;
                    } );
            }
        };

        namespace Detail
        {
            inline bool HoldsZero( const StringView& string ) noexcept
            {
                for ( char c : string )
                {
                    if ( c == 0 )
                    {
                        return true;
                    }
                }

                return false;
            }
        } // namespace Detail

        inline Status ValidateTraced( const Value& value, const double&, ReadPath& ) noexcept
        {
            return value.AsNumber() ? Status::kOk : Status::kInvalidValueType;
        }

        inline Status DeserializeTraced( const Value& value, double& data, ReadPath& ) noexcept
        {
            const double* number = value.AsNumber();

            if ( !number )
            {
                return Status::kInvalidValueType;
            }

            data = *number;

            return Status::kOk;
        }

        inline Status ValidateTraced( const Value& value, const bool&, ReadPath& ) noexcept
        {
            return value.AsBool() ? Status::kOk : Status::kInvalidValueType;
        }

        inline Status DeserializeTraced( const Value& value, bool& data, ReadPath& ) noexcept
        {
            const bool* boolean = value.AsBool();

            if ( !boolean )
            {
                return Status::kInvalidValueType;
            }

            data = *boolean;

            return Status::kOk;
        }

        inline Status ValidateTraced( const Value& value, const Null&, ReadPath& ) noexcept
        {
            return value.IsNull() ? Status::kOk : Status::kInvalidValueType;
        }

        inline Status DeserializeTraced( const Value& value, Null&, ReadPath& ) noexcept
        {
            return value.IsNull() ? Status::kOk : Status::kInvalidValueType;
        }

        // A String<> ends at its first zero, so a string that holds one fails with kStringHoldsZero.
        TERATHON_API Status ValidateTraced( const Value& value, const String<>& data, ReadPath& path ) noexcept;
        TERATHON_API Status DeserializeTraced( const Value& value, String<>& data, ReadPath& path ) noexcept;

        template <class T>
        Status ValidateTraced( const Value& value, const Array<T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status DeserializeTraced( const Value& value, Array<T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status ValidateTraced( const Value& value, const ObjectMap<T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status DeserializeTraced( const Value& value, ObjectMap<T>& data, ReadPath& path ) noexcept;

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        TERATHON_API Status ValidateTraced( const Value& value, const std::string& data, ReadPath& path ) noexcept;
        TERATHON_API Status DeserializeTraced( const Value& value, std::string& data, ReadPath& path ) noexcept;

        template <class T>
        Status ValidateTraced( const Value& value, const std::vector<T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status DeserializeTraced( const Value& value, std::vector<T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status ValidateTraced( const Value& value, const std::map<std::string, T>& data, ReadPath& path ) noexcept;

        template <class T>
        Status DeserializeTraced( const Value& value, std::map<std::string, T>& data, ReadPath& path ) noexcept;

#endif

        namespace Detail
        {
            using Json::Detail::DeclVal;
            using Json::Detail::IsDetectedExact;

            template <class T>
            using TracedValidateMember = decltype( DeclVal<const T&>().ValidateTraced( DeclVal<const Value&>(), DeclVal<ReadPath&>() ) );

            template <class T>
            using PlainValidateMember = decltype( DeclVal<const T&>().Validate( DeclVal<const Value&>() ) );

            template <class T>
            using TracedValidate = decltype( ValidateTraced( DeclVal<const Value&>(), DeclVal<const T&>(), DeclVal<ReadPath&>() ) );

            template <class T>
            using TracedDeserializeMember = decltype( DeclVal<T&>().DeserializeTraced( DeclVal<const Value&>(), DeclVal<ReadPath&>() ) );

            template <class T>
            using PlainDeserializeMember = decltype( DeclVal<T&>().Deserialize( DeclVal<const Value&>() ) );

            template <class T>
            using TracedDeserialize = decltype( DeserializeTraced( DeclVal<const Value&>(), DeclVal<T&>(), DeclVal<ReadPath&>() ) );
        } // namespace Detail

        // Validates or deserializes through the traced function of a type when it has one, and otherwise through its
        // own Validate or Deserialize, which adds nothing to the path.
        template <class T>
        Status ValidateValue( const Value& value, const T& data, ReadPath& path ) noexcept
        {
            if constexpr ( Detail::IsDetectedExact<Status, Detail::TracedValidateMember, T>::Value )
            {
                return data.ValidateTraced( value, path );
            }
            else if constexpr ( Detail::IsDetectedExact<Status, Detail::PlainValidateMember, T>::Value )
            {
                return Detail::MayThrow( [ & ]() { return data.Validate( value ); } );
            }
            else if constexpr ( Detail::IsDetectedExact<Status, Detail::TracedValidate, T>::Value )
            {
                return ValidateTraced( value, data, path );
            }
            else
            {
                return Detail::MayThrow( [ & ]() { return Validate( value, data ); } );
            }
        }

        template <class T>
        Status DeserializeValue( const Value& value, T& data, ReadPath& path ) noexcept
        {
            if constexpr ( Detail::IsDetectedExact<Status, Detail::TracedDeserializeMember, T>::Value )
            {
                return data.DeserializeTraced( value, path );
            }
            else if constexpr ( Detail::IsDetectedExact<Status, Detail::PlainDeserializeMember, T>::Value )
            {
                return Detail::MayThrow( [ & ]() { return data.Deserialize( value ); } );
            }
            else if constexpr ( Detail::IsDetectedExact<Status, Detail::TracedDeserialize, T>::Value )
            {
                return DeserializeTraced( value, data, path );
            }
            else
            {
                return Detail::MayThrow( [ & ]() { return Deserialize( value, data ); } );
            }
        }

        template <class T>
        Status ValidateTraced( const Value& value, const Array<T>&, ReadPath& path ) noexcept
        {
            ArrayView array = value.AsArray();

            if ( !array )
            {
                return Status::kInvalidValueType;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    const T element = T();

                    for ( uint32 a = 0; a < array.GetElementCount(); a++ )
                    {
                        Status status = ValidateValue( array[ a ], element, path );

                        if ( status != Status::kOk )
                        {
                            path.AddIndex( a );
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status DeserializeTraced( const Value& value, Array<T>& data, ReadPath& path ) noexcept
        {
            ArrayView array = value.AsArray();

            if ( !array )
            {
                return Status::kInvalidValueType;
            }

            Status status = Detail::MayThrow(
                [ & ]()
                {
                    data.PurgeArray();
                    data.ReserveArrayElementCount( int32( array.GetElementCount() ) );

                    for ( uint32 a = 0; a < array.GetElementCount(); a++ )
                    {
                        T      element;
                        Status elementStatus = DeserializeValue( array[ a ], element, path );

                        if ( elementStatus != Status::kOk )
                        {
                            path.AddIndex( a );
                            return elementStatus;
                        }

                        data.AppendArrayElement( static_cast<T&&>( element ) );
                    }

                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                data.PurgeArray();
            }

            return status;
        }

        template <class T>
        Status ValidateTraced( const Value& value, const ObjectMap<T>&, ReadPath& path ) noexcept
        {
            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    const T element = T();

                    for ( const Member& member : object )
                    {
                        Status status = Detail::HoldsZero( member.GetName() ) ? Status::kStringHoldsZero : ValidateValue( member.GetValue(), element, path );

                        if ( status != Status::kOk )
                        {
                            path.AddName( member.GetName().GetText(), member.GetName().GetLength() );
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status DeserializeTraced( const Value& value, ObjectMap<T>& data, ReadPath& path ) noexcept
        {
            using ElementType = typename ObjectMap<T>::ElementType;

            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            Status status = Detail::MayThrow(
                [ & ]()
                {
                    data.PurgeMap();

                    for ( const Member& member : object )
                    {
                        const StringView& name          = member.GetName();
                        ElementType*      element       = nullptr;
                        Status            elementStatus = Detail::HoldsZero( name ) ? Status::kStringHoldsZero : Status::kOk;

                        if ( elementStatus == Status::kOk )
                        {
                            elementStatus = Detail::MayThrow(
                                [ & ]()
                                {
                                    element       = new ElementType;
                                    element->name = Terathon::String<>( name.GetText(), int32( name.GetLength() ) );

                                    return DeserializeValue( member.GetValue(), element->data, path );
                                } );
                        }

                        if ( elementStatus != Status::kOk )
                        {
                            delete element;
                            path.AddName( name.GetText(), name.GetLength() );

                            return elementStatus;
                        }

                        data.InsertMapElement( element );
                    }

                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                data.PurgeMap();
            }

            return status;
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        template <class T>
        Status ValidateTraced( const Value& value, const std::vector<T>&, ReadPath& path ) noexcept
        {
            return ValidateTraced( value, Terathon::Array<T>(), path );
        }

        template <class T>
        Status DeserializeTraced( const Value& value, std::vector<T>& data, ReadPath& path ) noexcept
        {
            ArrayView array = value.AsArray();

            if ( !array )
            {
                return Status::kInvalidValueType;
            }

            Status status = Detail::MayThrow(
                [ & ]()
                {
                    data.clear();
                    data.reserve( array.GetElementCount() );

                    for ( uint32 a = 0; a < array.GetElementCount(); a++ )
                    {
                        T      element;
                        Status elementStatus = DeserializeValue( array[ a ], element, path );

                        if ( elementStatus != Status::kOk )
                        {
                            path.AddIndex( a );
                            return elementStatus;
                        }

                        data.push_back( static_cast<T&&>( element ) );
                    }

                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                data.clear();
            }

            return status;
        }

        template <class T>
        Status ValidateTraced( const Value& value, const std::map<std::string, T>&, ReadPath& path ) noexcept
        {
            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    const T element = T();

                    for ( const Member& member : object )
                    {
                        Status status = ValidateValue( member.GetValue(), element, path );

                        if ( status != Status::kOk )
                        {
                            path.AddName( member.GetName().GetText(), member.GetName().GetLength() );
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        // A std::map is left as it was when a value fails.
        template <class T>
        Status DeserializeTraced( const Value& value, std::map<std::string, T>& data, ReadPath& path ) noexcept
        {
            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    std::map<std::string, T> result;

                    for ( const Member& member : object )
                    {
                        const StringView& name   = member.GetName();
                        Status            status = DeserializeValue( member.GetValue(), result[ std::string( name.GetText(), name.GetLength() ) ], path );

                        if ( status != Status::kOk )
                        {
                            path.AddName( name.GetText(), name.GetLength() );
                            return status;
                        }
                    }

                    data.swap( result );

                    return Status::kOk;
                } );
        }

#endif

        // A member that the object does not have fails with kNameNotPresent, unless it is optional.
        template <class T>
        Status ValidateMember( const Value& value, const char* name, const T& data, bool optional, ReadPath& path ) noexcept
        {
            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            const Value* member = object.Find( name );

            if ( !member )
            {
                if ( optional )
                {
                    return Status::kOk;
                }

                path.AddName( name, true );
                return Status::kNameNotPresent;
            }

            Status status = ValidateValue( *member, data, path );

            if ( status != Status::kOk )
            {
                path.AddName( name );
            }

            return status;
        }

        template <class T>
        Status DeserializeMember( const Value& value, const char* name, T& data, bool optional, ReadPath& path ) noexcept
        {
            ObjectView object = value.AsObject();

            if ( !object )
            {
                return Status::kInvalidValueType;
            }

            const Value* member = object.Find( name );

            if ( !member )
            {
                if ( optional )
                {
                    return Status::kOk;
                }

                path.AddName( name, true );
                return Status::kNameNotPresent;
            }

            Status status = DeserializeValue( *member, data, path );

            if ( status != Status::kOk )
            {
                path.AddName( name );
            }

            return status;
        }

        inline Status ValidateProto( const Value&, ReadPath& ) noexcept
        {
            return Status::kOk;
        }

        template <class T, class... Args>
        Status ValidateProto( const Value& value, ReadPath& path, const T& data, Args&&... args ) noexcept
        {
            Status status = ValidateValue( value, data, path );

            return ( status == Status::kOk ) ? ValidateProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status ValidateProto( const Value& value, ReadPath& path, const char* name, const T& data, Args&&... args ) noexcept
        {
            Status status = ValidateMember( value, name, data, false, path );

            return ( status == Status::kOk ) ? ValidateProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status ValidateProto( const Value& value, ReadPath& path, Json::Detail::Optional, const char* name, const T& data, Args&&... args ) noexcept
        {
            Status status = ValidateMember( value, name, data, true, path );

            return ( status == Status::kOk ) ? ValidateProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        inline Status DeserializeProto( const Value&, ReadPath& ) noexcept
        {
            return Status::kOk;
        }

        template <class T, class... Args>
        Status DeserializeProto( const Value& value, ReadPath& path, T& data, Args&&... args ) noexcept
        {
            Status status = DeserializeValue( value, data, path );

            return ( status == Status::kOk ) ? DeserializeProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status DeserializeProto( const Value& value, ReadPath& path, const char* name, T& data, Args&&... args ) noexcept
        {
            Status status = DeserializeMember( value, name, data, false, path );

            return ( status == Status::kOk ) ? DeserializeProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status DeserializeProto( const Value& value, ReadPath& path, Json::Detail::Optional, const char* name, T& data, Args&&... args ) noexcept
        {
            Status status = DeserializeMember( value, name, data, true, path );

            return ( status == Status::kOk ) ? DeserializeProto( value, path, Json::Detail::Forward<Args>( args )... ) : status;
        }

        // For the types that have a traced function, and for custom Validate and Deserialize functions to call.
        template <class T, class = Detail::TracedValidate<T>>
        Status Validate( const Value& value, const T& data ) noexcept
        {
            ReadPath path;
            return ValidateValue( value, data, path );
        }

        template <class T, class = Detail::TracedDeserialize<T>>
        Status Deserialize( const Value& value, T& data ) noexcept
        {
            ReadPath path;
            return DeserializeValue( value, data, path );
        }

        template <class T>
        Status Validate( const Value& value, const char* name, const T& data ) noexcept
        {
            ReadPath path;
            return ValidateMember( value, name, data, false, path );
        }

        template <class T>
        Status Deserialize( const Value& value, const char* name, T& data ) noexcept
        {
            ReadPath path;
            return DeserializeMember( value, name, data, false, path );
        }

        template <class T>
        Status Validate( const StructuredData& document, const T& data ) noexcept
        {
            if ( !document.GetRoot() )
            {
                return Status::kInvalidStructuredData;
            }

            ReadPath path;
            return ValidateValue( *document.GetRoot(), data, path );
        }

        template <class T>
        Status StructuredData::DeserializeTo( T& data ) const noexcept
        {
            if ( !root )
            {
                return Status::kInvalidStructuredData;
            }

            ReadPath path;
            return DeserializeValue( *root, data, path );
        }

        struct ReadResult
        {
            Status   status      = Status::kOk;
            int32    errorLine   = 0;
            int32    errorColumn = 0;
            String<> path;
        };

        // Writes the path of a failed read, such as "meshes[2].name", and finds the line and column in the text of the
        // value at fault, or of the object that misses a member.
        TERATHON_API void LocateReadFailure( const char* text, uint64 length, const ParseOptions& options, const ReadPath& path, ReadResult& result ) noexcept;

        TERATHON_API Status ReadTextFile( const char* fileName, Array<char>& text ) noexcept;

        // Parses a text and deserializes it into data in one call. A failure reports the path of the value at fault and
        // its line and column.
        template <class T>
        ReadResult Read( const char* text, uint64 length, T& data, const ParseOptions& options = ParseOptions() ) noexcept
        {
            ReadResult     result;
            StructuredData document;
            ParseResult    parseResult = document.ParseText( text, length, options );

            if ( parseResult.status != Status::kOk )
            {
                result.status      = parseResult.status;
                result.errorLine   = parseResult.errorLine;
                result.errorColumn = parseResult.errorColumn;

                return result;
            }

            ReadPath path;
            result.status = DeserializeValue( *document.GetRoot(), data, path );

            if ( result.status != Status::kOk )
            {
                LocateReadFailure( text, length, options, path, result );
            }

            return result;
        }

        template <class T>
        ReadResult Read( const char* fileName, T& data, const ParseOptions& options = ParseOptions() ) noexcept
        {
            Terathon::Array<char> text;
            ReadResult            result;

            result.status = ReadTextFile( fileName, text );

            return ( result.status == Status::kOk ) ? Read( text.begin(), uint64( text.GetArrayElementCount() ), data, options ) : result;
        }

        class Builder;

        // A value that Serialize writes into a document. It is valid while the array or object that holds it is open,
        // which is until the Serialize call that is given it, or that adds it, returns. A call that fails fails the
        // build, and a node that AppendElement or AddMember fails to give is left empty, so that its own calls fail.
        class Node
        {
            friend class Builder;
            friend class StructuredData;

        public:
            TERATHON_API Status SetNull() noexcept;
            TERATHON_API Status SetBool( bool value ) noexcept;
            TERATHON_API Status SetNumber( double value ) noexcept;
            TERATHON_API Status SetString( const char* text, uint32 length ) noexcept;

            // Opening the array or object a node already is does nothing, so that structures can share one object.
            TERATHON_API Status SetArray() noexcept;
            TERATHON_API Status SetObject() noexcept;

            TERATHON_API Status AppendElement( Node& element ) noexcept;
            TERATHON_API Status AddMember( const char* name, uint32 length, Node& member ) noexcept;

            Status AddMember( const char* name, Node& member ) noexcept
            {
                return AddMember( name, Detail::GetTextLength( name ), member );
            }

            // For SerializeValue, which closes the arrays and objects that a Serialize call opened.
            TERATHON_API uint32 GetOpenCount() const noexcept;
            TERATHON_API Status CloseSince( uint32 openCount ) noexcept;

        private:
            Builder* builder = nullptr;
            uint64   offset  = 0;
        };

        inline Status Serialize( Node node, double data ) noexcept
        {
            return node.SetNumber( data );
        }

        inline Status Serialize( Node node, bool data ) noexcept
        {
            return node.SetBool( data );
        }

        inline Status Serialize( Node node, const Null& ) noexcept
        {
            return node.SetNull();
        }

        // Copies a value of this document or another.
        TERATHON_API Status Serialize( Node node, const Value& value ) noexcept;

        inline Status Serialize( Node node, const String<>& data ) noexcept
        {
            return node.SetString( data, uint32( data.GetStringLength() ) );
        }

        template <class T>
        Status Serialize( Node node, const Array<T>& data ) noexcept;

        template <class T>
        Status Serialize( Node node, const ObjectMap<T>& data ) noexcept;

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        inline Status Serialize( Node node, const std::string& data ) noexcept
        {
            return node.SetString( data.data(), uint32( data.size() ) );
        }

        template <class T>
        Status Serialize( Node node, const std::vector<T>& data ) noexcept;

        template <class T>
        Status Serialize( Node node, const std::map<std::string, T>& data ) noexcept;

#endif

        namespace Detail
        {
            template <class T>
            using SerializeMember = decltype( DeclVal<const T&>().Serialize( DeclVal<Node>() ) );

            template <class T>
            Status SerializeInto( Node node, const T& data ) noexcept
            {
                if constexpr ( IsDetectedExact<Status, SerializeMember, T>::Value )
                {
                    return Detail::MayThrow( [ & ]() { return data.Serialize( node ); } );
                }
                else
                {
                    return Detail::MayThrow( [ & ]() { return Serialize( node, data ); } );
                }
            }
        } // namespace Detail

        // Serializes through a type's member Serialize, or its free one, and closes the arrays and objects it opened.
        template <class T>
        Status SerializeValue( Node node, const T& data ) noexcept
        {
            uint32 openCount = node.GetOpenCount();
            Status status    = Detail::SerializeInto( node, data );

            return ( status == Status::kOk ) ? node.CloseSince( openCount ) : status;
        }

        template <class T>
        Status Serialize( Node node, const Array<T>& data ) noexcept
        {
            Status status = node.SetArray();

            for ( const T& item : data )
            {
                Node element;

                status = ( status == Status::kOk ) ? node.AppendElement( element ) : status;
                status = ( status == Status::kOk ) ? SerializeValue( element, item ) : status;
            }

            return status;
        }

        template <class T>
        Status Serialize( Node node, const ObjectMap<T>& data ) noexcept
        {
            Status status = node.SetObject();

            for ( const auto* element : data )
            {
                Node member;

                status = ( status == Status::kOk ) ? node.AddMember( element->name, uint32( element->name.GetStringLength() ), member ) : status;
                status = ( status == Status::kOk ) ? SerializeValue( member, element->data ) : status;
            }

            return status;
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        template <class T>
        Status Serialize( Node node, const std::vector<T>& data ) noexcept
        {
            Status status = node.SetArray();

            for ( const T& item : data )
            {
                Node element;

                status = ( status == Status::kOk ) ? node.AppendElement( element ) : status;
                status = ( status == Status::kOk ) ? SerializeValue( element, item ) : status;
            }

            return status;
        }

        template <class T>
        Status Serialize( Node node, const std::map<std::string, T>& data ) noexcept
        {
            Status status = node.SetObject();

            for ( const auto& element : data )
            {
                Node member;

                status = ( status == Status::kOk ) ? node.AddMember( element.first.data(), uint32( element.first.size() ), member ) : status;
                status = ( status == Status::kOk ) ? SerializeValue( member, element.second ) : status;
            }

            return status;
        }

#endif

        inline Status SerializeProto( Node ) noexcept
        {
            return Status::kOk;
        }

        // A structure without a name adds its members to the object that the structure around it writes.
        template <class T, class... Args>
        Status SerializeProto( Node node, const T& data, Args&&... args ) noexcept
        {
            Status status = Detail::SerializeInto( node, data );

            return ( status == Status::kOk ) ? SerializeProto( node, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status SerializeProto( Node node, const char* name, const T& data, Args&&... args ) noexcept
        {
            Node   member;
            Status status = node.SetObject();

            status = ( status == Status::kOk ) ? node.AddMember( name, member ) : status;
            status = ( status == Status::kOk ) ? SerializeValue( member, data ) : status;

            return ( status == Status::kOk ) ? SerializeProto( node, Json::Detail::Forward<Args>( args )... ) : status;
        }

        template <class T, class... Args>
        Status SerializeProto( Node node, Json::Detail::Optional, const char* name, const T& data, Args&&... args ) noexcept
        {
            return SerializeProto( node, name, data, Json::Detail::Forward<Args>( args )... );
        }

        template <class T>
        Status StructuredData::Build( const Edit& edit, const T& data ) noexcept
        {
            Node   rootNode;
            Status status = BeginBuild( edit, rootNode );

            status = ( status == Status::kOk ) ? SerializeValue( rootNode, data ) : status;

            return EndBuild( edit, rootNode, status );
        }

        template <class T>
        Status StructuredData::SerializeFrom( const T& data ) noexcept
        {
            return Build( Edit { EditKind::kDocument, nullptr, nullptr, 0 }, data );
        }

        template <class T>
        Status StructuredData::SerializeAt( const Value& target, const T& data ) noexcept
        {
            return Build( Edit { EditKind::kReplace, &target, nullptr, 0 }, data );
        }

        template <class T>
        Status StructuredData::AppendElement( const Value& array, const T& data ) noexcept
        {
            return Build( Edit { EditKind::kAppend, &array, nullptr, 0 }, data );
        }

        template <class T>
        Status StructuredData::AddMember( const Value& object, const char* name, uint32 length, const T& data ) noexcept
        {
            return Build( Edit { EditKind::kAddMember, &object, name, length }, data );
        }

    } // namespace Json
} // namespace C4

#define DEFINE_JSON4C4_FUNCTIONS( JSON4C4TYPE, JSON4C4PROTO )                                                                                                  \
    inline C4::Json::Status ValidateTraced( const C4::Json::Value& value, const JSON4C4TYPE& object, C4::Json::ReadPath& path ) noexcept                       \
    {                                                                                                                                                          \
        return C4::Json::ValidateProto( value, path, JSON4C4PROTO );                                                                                           \
    }                                                                                                                                                          \
    inline C4::Json::Status DeserializeTraced( const C4::Json::Value& value, JSON4C4TYPE& object, C4::Json::ReadPath& path ) noexcept                          \
    {                                                                                                                                                          \
        return C4::Json::DeserializeProto( value, path, JSON4C4PROTO );                                                                                        \
    }                                                                                                                                                          \
    inline C4::Json::Status Serialize( C4::Json::Node node, const JSON4C4TYPE& object ) noexcept                                                               \
    {                                                                                                                                                          \
        C4::Json::Status status = node.SetObject();                                                                                                            \
        return ( status == C4::Json::Status::kOk ) ? C4::Json::SerializeProto( node, JSON4C4PROTO ) : status;                                                  \
    }

#define DEFINE_JSON4C4_MEMBER_FUNCTIONS( JSON4C4PROTO )                                                                                                        \
    inline C4::Json::Status ValidateTraced( const C4::Json::Value& value, C4::Json::ReadPath& path ) const noexcept                                            \
    {                                                                                                                                                          \
        return C4::Json::ValidateProto( value, path, JSON4C4PROTO );                                                                                           \
    }                                                                                                                                                          \
    inline C4::Json::Status DeserializeTraced( const C4::Json::Value& value, C4::Json::ReadPath& path ) noexcept                                               \
    {                                                                                                                                                          \
        return C4::Json::DeserializeProto( value, path, JSON4C4PROTO );                                                                                        \
    }                                                                                                                                                          \
    inline C4::Json::Status Serialize( C4::Json::Node node ) const noexcept                                                                                    \
    {                                                                                                                                                          \
        C4::Json::Status status = node.SetObject();                                                                                                            \
        return ( status == C4::Json::Status::kOk ) ? C4::Json::SerializeProto( node, JSON4C4PROTO ) : status;                                                  \
    }

#if defined( _MSC_VER )

#    pragma warning( pop )

#endif

#endif
