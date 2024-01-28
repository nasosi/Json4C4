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

#include "TSArray.h"
#include "TSMap.h"
#include "TSString.h"

#if defined( _MSC_VER )

#    pragma warning( push )
#    pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

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
    class File;

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
        } // namespace Detail

        enum class Status : unsigned int
        {
            kOk = 0,
            KFileOpenError,
            KFileReadError,
            kTextBufferContentsInvalid,
            kUnterminatedTextBuffer,
            kExpectedOpeningCurlyBraces,
            kExpectedClosingCurlyBracesOrComma,
            kExpectedOpeningSquareBrackets,
            kExpectedClosingSquareBracketsOrComma,
            kStringIllegalChar,
            kStringIllegalEscape,
            kExpectedColon,
            kExpectedBeginingDoubleQuotes,
            kExpectedEndingDoubleQuotes,
            kFloatInvalid,
            kExpectedTrueOrFalse,
            kExpectedNull,
            kExpectedJsonValue,
            kExpectedEndOfFile,
            kPrematureNullTerminator,
            kNumbersCannotHaveLeadingZeros,
            kInvalidStructuredData,
            kMissingObjectElement,
            kException,
            kInvalidValueType,
            kNameNotPresent
        };

        TERATHON_API String<256> StatusToString( const Status& parseResult ) noexcept;

        struct ParseResult
        {
            Status status      = Status::kOk;
            int32  errorLine   = 0;
            int32  errorColumn = 0;
        };

        template <class Func>
        Status MayThrow( Func&& func ) noexcept
        {
            // When TERATHON_NO_SYSTEM is defined, operator new doesn't throw, so there is no much point in including try/catch
#if ( ( ( defined( __GNUC__ ) || defined( __GNUG__ ) || defined( __clang__ ) ) && defined( __cpp_exceptions ) ) || defined( _MSC_VER ) ) &&                    \
    !defined( TERATHON_NO_SYSTEM )
            try
#endif
            {
                return func();
            }
#if ( ( ( defined( __GNUC__ ) || defined( __GNUG__ ) || defined( __clang__ ) ) && defined( __cpp_exceptions ) ) || defined( _MSC_VER ) ) &&                    \
    !defined( TERATHON_NO_SYSTEM )
            catch ( ... )
            {
            }
            return Status::kException;
#endif
        }

        TERATHON_API const char* GetVersion();

        // Basic Types :
        // String<>
        // double
        // bool
        // and:
        struct Null
        {
        };

        class StringValue;
        class NumberValue;
        class NullValue;
        class BoolValue;
        class ArrayValue;
        class ObjectValue;

        template <class ValueType>
        class ObjectMapElement : public Terathon::MapElement<ObjectMapElement<ValueType>>
        {
        public:
            using KeyType = Terathon::String<>;

            KeyType   name;
            ValueType data = ValueType {};

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

        class Value : public Terathon::MapElement<Value>
        {
        private:
            inline virtual String<>* GetStringData() noexcept
            {
                return nullptr;
            }

            inline virtual const String<>* GetStringData() const noexcept
            {
                return nullptr;
            };

            inline virtual double* GetDoubleData() noexcept
            {
                return nullptr;
            };

            inline virtual const double* GetDoubleData() const noexcept
            {
                return nullptr;
            };

            inline virtual bool* GetBoolData() noexcept
            {
                return nullptr;
            };

            inline virtual const bool* GetBoolData() const noexcept
            {
                return nullptr;
            };

            inline virtual Null* GetJsonNullData() noexcept
            {
                return nullptr;
            };

            inline virtual const Null* GetJsonNullData() const noexcept
            {
                return nullptr;
            };

            inline virtual Array<Value*>* GetJsonValueArrayData() noexcept
            {
                return nullptr;
            };

            inline virtual const Array<Value*>* GetJsonValueArrayData() const noexcept
            {
                return nullptr;
            };

        public:
            using KeyType = String<>;

            String<> name;

            TERATHON_API inline const KeyType& GetKey() const noexcept
            {
                return name;
            };

            TERATHON_API virtual ~Value() noexcept;

            inline virtual ObjectValue* AsJsonObjectValue() noexcept
            {
                return nullptr;
            };

            inline virtual const ObjectValue* AsJsonObjectValue() const noexcept
            {
                return nullptr;
            }

            virtual Status Write( File& file, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const = 0;

            template <class T>
            T* GetDataAsPointerTo() noexcept
            {
                constexpr bool AlwaysFalse = !Detail::IsSame<T, T>::Value;

                static_assert( AlwaysFalse, "Json4C4: Unsupported type passed to a process function." );
            }

            template <class T>
            const T* GetDataAsPointerTo() const noexcept
            {
                constexpr bool AlwaysFalse = !Detail::IsSame<T, T>::Value;

                static_assert( AlwaysFalse, "Json4C4: Unsupported type passed to a process function." );
            }
        };

        template <>
        inline String<>* Value::GetDataAsPointerTo<String<>>() noexcept
        {
            return this->GetStringData();
        }

        template <>
        inline const String<>* Value::GetDataAsPointerTo<String<>>() const noexcept
        {
            return this->GetStringData();
        }

        template <>
        inline double* Value::GetDataAsPointerTo<double>() noexcept
        {
            return this->GetDoubleData();
        }

        template <>
        inline const double* Value::GetDataAsPointerTo<double>() const noexcept
        {
            return this->GetDoubleData();
        }

        template <>
        inline bool* Value::GetDataAsPointerTo<bool>() noexcept
        {
            return this->GetBoolData();
        }

        template <>
        inline const bool* Value::GetDataAsPointerTo<bool>() const noexcept
        {
            return this->GetBoolData();
        }

        template <>
        inline Null* Value::GetDataAsPointerTo<Null>() noexcept
        {
            return this->GetJsonNullData();
        }

        template <>
        inline const Null* Value::GetDataAsPointerTo<Null>() const noexcept
        {
            return this->GetJsonNullData();
        }

        template <>
        inline Array<Value*>* Value::GetDataAsPointerTo<Array<Value*>>() noexcept
        {
            return this->GetJsonValueArrayData();
        }

        template <>
        inline const Array<Value*>* Value::GetDataAsPointerTo<Array<Value*>>() const noexcept
        {
            return this->GetJsonValueArrayData();
        }

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
                NoneSuch( NoneSuch const& )       = delete;
                void operator=( NoneSuch const& ) = delete;
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

            template <class T>
            using ValidateMemberOperator = decltype( DeclVal<T&>().Validate( DeclVal<const Value*>() ) );

            template <class T>
            using SerializeMemberOperator = decltype( DeclVal<T&>().Serialize( DeclVal<Value*>() ) );

            template <class T>
            using DeserialzeMemberOperator = decltype( DeclVal<T&>().Deserialize( DeclVal<const Value*>() ) );

            template <class T>
            using HasDeserializeMember = IsDetectedExact<Status, DeserialzeMemberOperator, T>;

            template <class T>
            using HasValidateMember = IsDetectedExact<Status, ValidateMemberOperator, T>;

            template <class T>
            using HasSerializeMember = IsDetectedExact<Status, SerializeMemberOperator, T>;
        } // namespace Detail

        class ArrayValue final : public Value
        {
        public:
            static Status Parse( ArrayValue* jsonArray, const char*& text ) noexcept;

        public:
            using DataType = Array<Value*>;

            DataType data;

            TERATHON_API ~ArrayValue() override;

            TERATHON_API Array<Value*>* GetJsonValueArrayData() noexcept override;

            TERATHON_API const Array<Value*>* GetJsonValueArrayData() const noexcept override;

            TERATHON_API Status Write( File& file, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const override;
        };

        class ObjectValue final : public Value, public Map<Value>
        {
        private:
            Array<String<>> insertionOrder;

        public:
            using Map<Value>::RemoveAllMapElements;
            using Map<Value>::begin;
            using Map<Value>::end;

        public:
            static Status Parse( ObjectValue* jsonObjectValue, const char*& text ) noexcept;

        public:
            TERATHON_API ~ObjectValue() noexcept override;

            TERATHON_API ObjectValue*       AsJsonObjectValue() noexcept override;
            TERATHON_API const ObjectValue* AsJsonObjectValue() const noexcept override;

            TERATHON_API const String<>* FindString( const char* name ) const noexcept;
            TERATHON_API const double*   FindNumber( const char* name ) const noexcept;
            TERATHON_API const bool*     FindBool( const char* name ) const noexcept;
            TERATHON_API const Null*     FindJsonNull( const char* name ) const noexcept;
            TERATHON_API const Array<Value*>* FindJsonValueArray( const char* name ) const noexcept;
            TERATHON_API const ObjectValue*   FindJsonObjectValue( const char* name ) const noexcept;

            TERATHON_API bool InsertAccountedMapElement( Value* element );

            TERATHON_API Status Write( File& file, uint32 indentationLength, const char indentationChar, Array<char>& indentationCharArray ) const override;
        };

        namespace Detail
        {

            template <class T>
            Value* CreateValueFor( const T& ) noexcept( false )
            {
                return new ObjectValue;
            }

            template <class T>
            Value* CreateValueFor( const Array<T>& ) noexcept( false )
            {
                return new ArrayValue;
            }

            template <>
            TERATHON_API Value* CreateValueFor( const String<>& ) noexcept( false );

            template <>
            TERATHON_API Value* CreateValueFor( const double& ) noexcept( false );

            template <>
            TERATHON_API Value* CreateValueFor( const bool& ) noexcept( false );

            template <>
            TERATHON_API Value* CreateValueFor( const Null& ) noexcept( false );

        } // namespace Detail

        class StructuredData
        {
        private:
            Value* rootJsonValue = nullptr;

        public:
            TERATHON_API StructuredData() = default;
            TERATHON_API ~StructuredData() noexcept;

            StructuredData( const StructuredData& ) = delete;
            StructuredData( StructuredData& )       = delete;
            void operator=( const StructuredData& ) = delete;
            void operator=( StructuredData& )       = delete;

            TERATHON_API ParseResult Parse( const char* fileName ) noexcept;
            TERATHON_API ParseResult Parse( const Array<char>& nullTerminatedTextBuffer ) noexcept;

            TERATHON_API Status Write( const char* fileName, const uint32 indentationLength = 2, const char indentationChar = ' ' ) noexcept;

            TERATHON_API Value*       GetRootJsonValue() noexcept;
            TERATHON_API const Value* GetRootJsonValue() const noexcept;

            template <class T>
            Status DeserializeTo( T& data ) noexcept
            {
                return MayThrow(
                    [ & ]()
                    {
                        if constexpr ( Detail::HasDeserializeMember<T>::Value )
                        {
                            return data.Deserialize( this->GetRootJsonValue() );
                        }
                        else
                        {
                            return Deserialize( this->GetRootJsonValue(), data );
                        }
                    } );
            }

            template <class T>
            Status SerializeFrom( const T& data ) noexcept
            {
                if ( rootJsonValue != nullptr )
                {
                    delete rootJsonValue;
                }

                Status status = MayThrow(
                    [ & ]()
                    {
                        rootJsonValue = Detail::CreateValueFor( data );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    return status;
                }

                return MayThrow(
                    [ & ]()
                    {
                        if constexpr ( Detail::HasSerializeMember<T>::Value )
                        {
                            return data.Serialize( this->GetRootJsonValue() );
                        }
                        else
                        {
                            return Serialize( this->GetRootJsonValue(), data );
                        }
                    } );
            }
        };

        template <class T>
        Status Validate( const StructuredData& sd, const T& data ) noexcept
        {
            return MayThrow(
                [ & ]()
                {
                    if constexpr ( Detail::HasValidateMember<T>::Value )
                    {
                        return data.Validate( sd.GetRootJsonValue() );
                    }
                    else
                    {
                        return Validate( sd.GetRootJsonValue(), data );
                    }
                } );
        }

        TERATHON_API String<256> ParseResultToString( const ParseResult& parseResult ) noexcept;

        template <class T>
        Status Validate( const Value* value, const char* name, const T& data ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            const Value* associateValue = objectValue->FindMapElement( name );

            if ( !associateValue )
            {
                return Status::kNameNotPresent;
            }

            return MayThrow( [ & ]() { return Validate( associateValue, data ); } );
        }

        template <class T>
        Status Deserialize( const Value* value, const char* name, T& data ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            const Value* associateValue = objectValue->FindMapElement( name );

            if ( !associateValue )
            {
                return Status::kNameNotPresent;
            }

            return MayThrow( [ & ]() { return Deserialize( associateValue, data ); } );
        }

        template <class T>
        Status Serialize( Value* value, const char* name, const T& data ) noexcept
        {
            ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( objectValue )
            {

                Value* associateValue = nullptr;

                Status status = MayThrow(
                    [ & ]()
                    {
                        associateValue       = Detail::CreateValueFor( data );
                        associateValue->name = name;
                        Serialize( associateValue, data );

                        objectValue->InsertAccountedMapElement( associateValue );

                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    if ( associateValue != nullptr )
                    {
                        delete associateValue;
                    }
                    return status;
                }

                return Status::kOk;
            }

            Array<Value*>* valueArray = value->GetDataAsPointerTo<Array<Value*>>();
            if ( valueArray != nullptr )
            {
                Value* element = nullptr;

                Status status = MayThrow(
                    [ & ]()
                    {
                        element = Detail::CreateValueFor( data );
                        valueArray->AppendArrayElement( element );
                        return Status::kOk;
                    } );

                if ( status != Status::kOk )
                {
                    if ( element != nullptr )
                    {
                        delete element;
                    }
                    return status;
                }

                return MayThrow( [ & ]() { return Serialize( element, data ); } );
            }

            return Status::kInvalidValueType;
        }

        template <class T>
        Status Validate( const Value* value, const T& /*data*/ ) noexcept
        {
            const T* valueData = value->GetDataAsPointerTo<T>();

            return valueData ? Status::kOk : Status::kInvalidValueType;
        }

        template <class T>
        Status Deserialize( const Value* value, T& data ) noexcept
        {
            const T* valueData = value->GetDataAsPointerTo<T>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    if constexpr ( Detail::HasDeserializeMember<T>::Value )
                    {
                        return data.Deserialize( value );
                    }
                    else
                    {
                        data = *valueData;
                        return Status::kOk;
                    }
                } );
        }

        template <class T>
        Status Serialize( Value* value, const T& data ) noexcept
        {
            T* valueData = value->GetDataAsPointerTo<T>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    if constexpr ( Detail::HasSerializeMember<T>::Value )
                    {
                        return data.Serialize( value );
                    }
                    else
                    {
                        *valueData = data;

                        return Status::kOk;
                    }
                } );
        }

        template <class T>
        Status Validate( const Value* value, const Array<T>& /*data*/ ) noexcept
        {
            const Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    const T temp {};

                    Status status;

                    for ( Value* valueElement : *valueData )
                    {
                        if constexpr ( Detail::HasValidateMember<T>::Value )
                        {

                            status = temp.Validate( valueElement );
                        }
                        else
                        {
                            status = Validate( valueElement, temp );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Deserialize( const Value* value, Array<T>& data ) noexcept
        {
            const Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    data.PurgeArray();
                    data.ReserveArrayElementCount( valueData->GetArrayElementCount() );

                    Status status;

                    for ( Value* valueElement : *valueData )
                    {
                        T temp;
                        if constexpr ( Detail::HasDeserializeMember<T>::Value )
                        {
                            status = temp.Deserialize( valueElement );
                        }
                        else
                        {
                            status = Deserialize( valueElement, temp );
                        }

                        if ( status == Status::kOk )
                        {
                            data.AppendArrayElement( ( temp ) );
                        }
                        else
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Serialize( Value* value, const Array<T>& data ) noexcept
        {
            Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    valueData->PurgeArray();
                    valueData->ReserveArrayElementCount( data.GetArrayElementCount() );

                    for ( const T& element : data )
                    {
                        Value* value = nullptr;

                        Status status = MayThrow(
                            [ & ]()
                            {
                                value = Detail::CreateValueFor( element );
                                return Status::kOk;
                            } );

                        if ( status != Status::kOk )
                        {
                            return status;
                        }

                        valueData->AppendArrayElement( value );

                        if constexpr ( Detail::HasSerializeMember<T>::Value )
                        {
                            status = element.Serialize( value );
                        }
                        else
                        {
                            status = Serialize( value, element );
                        }

                        if ( status != Status::kOk )
                        {
                            delete value;
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Validate( const Value* value, const ObjectMap<T>& /*data*/ ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    Status status;
                    T      temp;

                    for ( const auto* objectValueMapElement : *objectValue )
                    {
                        if constexpr ( Detail::HasValidateMember<T>::Value )
                        {
                            status = temp.Validate( objectValueMapElement );
                        }
                        else
                        {
                            status = Validate( objectValueMapElement, temp );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }
                    return Status::kOk;
                } );
        }

        template <class T>
        Status Deserialize( const Value* value, ObjectMap<T>& data ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    using DataMapElementType = typename ObjectMap<T>::ElementType;
                    Status status;
                    bool   addedNew = false;

                    for ( const auto* objectValueMapElement : *objectValue )
                    {
                        DataMapElementType* dataMapElement = data.FindMapElement( objectValueMapElement->GetKey() );

                        addedNew = false;

                        if ( !dataMapElement )
                        {
                            status = MayThrow(
                                [ & ]()
                                {
                                    dataMapElement = new DataMapElementType;
                                    return Status::kOk;
                                } );

                            if ( status != Status::kOk )
                            {
                                return status;
                            }

                            dataMapElement->name = objectValueMapElement->GetKey();

                            addedNew = true;
                        }

                        if constexpr ( Detail::HasDeserializeMember<T>::Value )
                        {
                            status = dataMapElement->data.Deserialize( objectValueMapElement );
                        }
                        else
                        {
                            status = Deserialize( objectValueMapElement, dataMapElement->data );
                        }

                        if ( status != Status::kOk )
                        {
                            if ( addedNew )
                            {
                                delete dataMapElement;
                            }

                            return status;
                        }

                        status = MayThrow(
                            [ & ]()
                            {
                                data.InsertMapElement( dataMapElement );
                                return Status::kOk;
                            } );

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }
                    return Status::kOk;
                } );
        }

        template <class T>
        Status Serialize( Value* value, const ObjectMap<T>& data ) noexcept
        {
            ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    objectValue->RemoveAllMapElements();

                    T      temp;
                    Status status;

                    for ( const auto* dataMapElement : data )
                    {
                        Value* elem = Detail::CreateValueFor( temp );
                        elem->name  = dataMapElement->name;

                        if constexpr ( Detail::HasSerializeMember<T>::Value )
                        {
                            status = dataMapElement->data.Serialize( value );
                        }
                        else
                        {
                            status = Serialize( elem, dataMapElement->data );
                        }

                        if ( status != Status::kOk )
                        {
                            delete elem;
                            return status;
                        }

                        objectValue->InsertMapElement( elem );
                    }
                    return Status::kOk;
                } );
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        namespace Detail
        {
            template <>
            TERATHON_API Value* CreateValueFor( const std::string& );
        }

        template <>
        TERATHON_API Status Validate( const Value* value, const std::string& data ) noexcept;

        template <>
        TERATHON_API Status Deserialize( const Value* value, std::string& data ) noexcept;

        template <>
        TERATHON_API Status Serialize( Value* value, const std::string& data ) noexcept;

        template <class T>
        Status Validate( const Value* value, const std::vector<T>& /*data*/ ) noexcept
        {
            const Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    const T temp {};
                    Status  status;

                    for ( Value* value : *valueData )
                    {
                        if constexpr ( Detail::HasValidateMember<T>::Value )
                        {
                            status = temp->Validate( value );
                        }
                        else
                        {
                            status = Validate( value, temp );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Deserialize( const Value* value, std::vector<T>& data ) noexcept
        {
            const Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    data.clear();
                    data.reserve( valueData->GetArrayElementCount() );

                    if ( data.capacity() < std::size_t( valueData->GetArrayElementCount() ) )
                    {
                        return Status::kException;
                    }

                    Status status;

                    for ( Value* value : *valueData )
                    {
                        T temp;

                        if constexpr ( Detail::HasDeserializeMember<T>::Value )
                        {
                            status = temp->Deserialize( value );
                        }
                        else
                        {
                            status = Deserialize( value, temp );
                        }

                        if ( status == Status::kOk )
                        {
                            data.push_back( static_cast<T&&>( temp ) );
                        }
                        else
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Serialize( Value* value, const std::vector<T>& data ) noexcept
        {
            Array<Value*>* valueData = value->GetDataAsPointerTo<Array<Value*>>();

            if ( !valueData )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    valueData->PurgeArray();
                    valueData->ReserveArrayElementCount( int32( data.size() ) );

                    for ( const T& element : data )
                    {
                        Value* value = nullptr;

                        Status status = MayThrow(
                            [ & ]()
                            {
                                value = Detail::CreateValueFor( element );
                                return Status::kOk;
                            } );

                        if ( status != Status::kOk )
                        {
                            return status;
                        }

                        valueData->AppendArrayElement( value );

                        if constexpr ( Detail::HasSerializeMember<T>::Value )
                        {
                            status = element->Serialize( value );
                        }
                        else
                        {
                            status = Serialize( value, element );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Validate( const Value* value, const std::map<std::string, T>& /*data*/ ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    T      temp;
                    Status status;

                    for ( const auto* objectValueMapElement : *objectValue )
                    {
                        if constexpr ( Detail::HasValidateMember<T>::Value )
                        {
                            status = temp.Validate( objectValueMapElement );
                        }
                        else
                        {
                            status = Validate( objectValueMapElement, temp );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Deserialize( const Value* value, std::map<std::string, T>& data ) noexcept
        {
            const ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    Status status;

                    for ( const auto* objectValueMapElement : *objectValue )
                    {
                        T& dataMapElementData = data[ static_cast<const char*>( objectValueMapElement->GetKey() ) ];

                        if constexpr ( Detail::HasDeserializeMember<T>::Value )
                        {
                            status = dataMapElementData.Deserialize( objectValueMapElement );
                        }
                        else
                        {
                            status = Deserialize( objectValueMapElement, dataMapElementData );
                        }

                        if ( status != Status::kOk )
                        {
                            return status;
                        }
                    }

                    return Status::kOk;
                } );
        }

        template <class T>
        Status Serialize( Value* value, const std::map<std::string, T>& data ) noexcept
        {
            ObjectValue* objectValue = value->AsJsonObjectValue();

            if ( !objectValue )
            {
                return Status::kInvalidValueType;
            }

            return MayThrow(
                [ & ]()
                {
                    objectValue->RemoveAllMapElements();

                    T      temp;
                    Status status;

                    for ( auto const& [ dataMapElementKey, dataMapElementData ] : data )
                    {
                        Value* elem = Detail::CreateValueFor( temp );
                        elem->name  = dataMapElementKey.c_str();

                        if constexpr ( Detail::HasSerializeMember<T>::Value )
                        {
                            status = dataMapElementData.Serialize( value );
                        }
                        else
                        {
                            status = Serialize( elem, dataMapElementData );
                        }

                        if ( status != Status::kOk )
                        {
                            delete elem;
                            return status;
                        }

                        objectValue->InsertMapElement( elem );
                    }

                    return Status::kOk;
                } );
        }

#endif

        inline Status ValidateProto( const Value* ) noexcept
        {
            return Status::kOk;
        }

        template <class T, class... Args>
        Status ValidateProto( const Value* value, const T& data, Args&&... args ) noexcept
        {
            Status status = Validate( value, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return ValidateProto( value, Detail::Forward<Args>( args )... );
        }

        template <class T, class... Args>
        Status ValidateProto( const Value* value, const char* name, const T& data, Args&&... args ) noexcept
        {
            Status status = Validate( value, name, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return ValidateProto( value, Detail::Forward<Args>( args )... );
        }

        inline Status DeserializeProto( const Value* ) noexcept
        {
            return Status::kOk;
        }

        template <class T, class... Args>
        Status DeserializeProto( const Value* value, T& data, Args&&... args ) noexcept
        {
            Status status = Deserialize( value, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return DeserializeProto( value, Detail::Forward<Args>( args )... );
        }

        template <class T, class... Args>
        Status DeserializeProto( const Value* value, const char* name, T& data, Args&&... args ) noexcept
        {
            Status status = Deserialize( value, name, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return DeserializeProto( value, Detail::Forward<Args>( args )... );
        }

        inline Status SerializeProto( Value* ) noexcept
        {
            return Status::kOk;
        }

        template <class T, class... Args>
        Status SerializeProto( Value* value, const char* name, const T& data, Args&&... args ) noexcept
        {
            Status status = Serialize( value, name, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return SerializeProto( value, Detail::Forward<Args>( args )... );
        }

        template <class T, class... Args>
        Status SerializeProto( Value* value, const T& data, Args&&... args ) noexcept
        {
            Status status = Serialize( value, data );

            if ( status != Status::kOk )
            {
                return status;
            }

            return SerializeProto( value, Detail::Forward<Args>( args )... );
        }

    } // namespace Json
} // namespace C4

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::String<>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::String<>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::String<>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, double& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const double& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const double& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, bool& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const bool& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const bool& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::Null& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::Null& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::Null& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<Terathon::String<>>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<Terathon::String<>>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<double>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<bool>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, Terathon::Array<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const Terathon::Array<C4::Json::Null>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<Terathon::String<>>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<double>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<bool>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const C4::Json::ObjectMap<C4::Json::Null>& data ) noexcept;

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::string& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::string& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::string& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<std::string>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<std::string>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<double>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<bool>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::vector<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::vector<C4::Json::Null>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, std::string>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, std::string>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, double>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, bool>& data ) noexcept;

extern template TERATHON_API C4::Json::Status C4::Json::Deserialize( const C4::Json::Value* value, std::map<std::string, C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Validate( const C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;
extern template TERATHON_API C4::Json::Status C4::Json::Serialize( C4::Json::Value* value, const std::map<std::string, C4::Json::Null>& data ) noexcept;

#endif

#define DEFINE_JSON4C4_FUNCTIONS( JSON4C4TYPE, JSON4C4PROTO )                                                                                                  \
    inline C4::Json::Status Deserialize( const C4::Json::Value* sd, JSON4C4TYPE& object ) noexcept                                                             \
    {                                                                                                                                                          \
        return C4::Json::DeserializeProto( sd, JSON4C4PROTO );                                                                                                 \
    }                                                                                                                                                          \
    inline C4::Json::Status Validate( const C4::Json::Value* sd, const JSON4C4TYPE& object ) noexcept                                                          \
    {                                                                                                                                                          \
        return C4::Json::ValidateProto( sd, JSON4C4PROTO );                                                                                                    \
    }                                                                                                                                                          \
    inline C4::Json::Status Serialize( C4::Json::Value* sd, const JSON4C4TYPE& object ) noexcept                                                               \
    {                                                                                                                                                          \
        return C4::Json::SerializeProto( sd, JSON4C4PROTO );                                                                                                   \
    }

#define DEFINE_JSON4C4_MEMBER_FUNCTIONS( JSON4C4PROTO )                                                                                                        \
    inline C4::Json::Status Deserialize( const C4::Json::Value* sd ) noexcept                                                                                  \
    {                                                                                                                                                          \
        return C4::Json::DeserializeProto( sd, JSON4C4PROTO );                                                                                                 \
    }                                                                                                                                                          \
    inline C4::Json::Status Validate( const C4::Json::Value* sd ) const noexcept                                                                               \
    {                                                                                                                                                          \
        return C4::Json::ValidateProto( sd, JSON4C4PROTO );                                                                                                    \
    }                                                                                                                                                          \
    inline C4::Json::Status Serialize( C4::Json::Value* sd ) const noexcept                                                                                    \
    {                                                                                                                                                          \
        return C4::Json::SerializeProto( sd, JSON4C4PROTO );                                                                                                   \
    }

#if defined( _MSC_VER )

#    pragma warning( pop )

#endif

#endif
