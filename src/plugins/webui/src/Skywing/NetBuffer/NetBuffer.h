#ifdef _MSC_VER

#pragma once

#pragma warning(push)
#pragma warning(disable:4290) // warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

#endif

#ifndef _NWNCONNLIB_NETBUFFER_NETBUFFER_H
#define _NWNCONNLIB_NETBUFFER_NETBUFFER_H

#include "Skywing/Precomp.h"

namespace NWN
{

	class ExoParseBuffer
	{

	public:

		ExoParseBuffer(
			__in_bcount( ByteDataLength ) const void *ByteData,
			__in size_t ByteDataLength,
			__in_bcount_opt( BitDataLength ) const void *BitData,
			__in size_t BitDataLength
			);

		~ExoParseBuffer( );

		//
		// Reads a counted string from the wire.  If the length is 32 bits,
		// then it is treated as an unsigned value as far as where the length
		// bits come from.  Otherwise it is treated as a signed value, though
		// we return a blank string as per the NWN implementation if we get a
		// negative value.
		//

		bool
		ReadCExoString(
			__out std::string &String,
			__in size_t NumBits = 32
			);

		bool
		ReadCExoLocString(
			__out ExoLocString &String
			);

		//
		// Read a string with an unsigned 8-bit length prefix.
		//

		bool
		ReadSmallString(
			__out std::string &String,
			__in size_t NumBits = 8
			);

		//
		// Read a NWN1-style 16-byte CResRef.
		//

		bool
		ReadCResRef16(
			__out ResRef16 &Ref,
			__in size_t NumBytes = sizeof( ResRef16 )
			)
		{
			if (NumBytes > sizeof( ResRef16 ))
				NumBytes = sizeof( ResRef16 );

			return m_ByteStream.GetData( NumBytes, &Ref );
		}

		//
		// Read a NWN2-style 32-byte CResRef.
		//

		bool
		ReadCResRef32(
			__out ResRef32 &Ref,
			__in size_t NumBytes = sizeof( ResRef32 )
			)
		{
			if (NumBytes > sizeof( ResRef32 ))
				NumBytes = sizeof( ResRef32 );

			return m_ByteStream.GetData( NumBytes, &Ref );
		}

		inline
		bool
		ReadBOOL(
			__out bool &Value
			)
		{
			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, 1 ))
				return false;

			Value = Bits ? true : false;

			return true;
		}

		inline
		bool
		ReadCHAR(
			__out signed char &Value,
			__in size_t NumBits = 8
			)
		{
			if (NumBits == 8)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadSigned( Bits, NumBits ))
				return false;

			if ((Bits >> 32) == 0x80000000)
				Value = SCHAR_MIN;
			else
				Value = static_cast< signed char >( Bits & 0xFF );

			return true;
		}

		inline
		bool
		ReadSHORT(
			__out signed short &Value,
			__in size_t NumBits = 16
			)
		{
			if (NumBits == 16)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadSigned( Bits, NumBits ))
				return false;

			if ((Bits >> 32) == 0x80000000)
				Value = SHRT_MIN;
			else
				Value = static_cast< signed short >( Bits & 0xFFFF );

			return true;
		}

		inline
		bool
		ReadINT(
			__out signed int &Value,
			__in size_t NumBits = 32
			)
		{
			if (NumBits == 32)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadSigned( Bits, NumBits ))
				return false;

			if ((Bits >> 32) == 0x80000000)
				Value = INT_MIN;
			else
				Value = static_cast< signed int >( Bits & 0xFFFFFFFF );

			return true;
		}

		inline
		bool
		ReadINT64(
			__out signed __int64 &Value,
			__in size_t NumBits = 64
			)
		{
			if (NumBits == 64)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadSigned( Bits, NumBits ))
				return false;

			Value = static_cast< signed __int64 >( Bits );

			return true;
		}

		inline
		bool
		ReadBYTE(
			__out unsigned char &Value,
			__in size_t NumBits = 8
			)
		{
			if (NumBits == 8)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value = static_cast< unsigned char >( Bits & 0xFF );

			return true;
		}

		inline
		bool
		ReadWORD(
			__out unsigned short &Value,
			__in size_t NumBits = 16
			)
		{
			if (NumBits == 16)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value = static_cast< unsigned short >( Bits & 0xFFFF );

			return true;
		}

		inline
		bool
		ReadDWORD(
			__out unsigned long &Value,
			__in size_t NumBits = 32
			)
		{
			if (NumBits == 32)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value = static_cast< unsigned long >( Bits & 0xFFFFFFFF );

			return true;
		}

		inline
		bool
		ReadDWORD64(
			__out unsigned __int64 &Value,
			__in size_t NumBits = 64
			)
		{
			if (NumBits == 64)
				return m_ByteStream.GetField( Value );
			else
				return ReadUnsigned( Value, NumBits );
		}

		inline
		bool
		ReadFLOAT(
			__out float &Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			)
		{
			if ((NumBits == sizeof( float ) * 8) && (Scale == 1.0))
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value = static_cast< float >(
				static_cast< unsigned long >( Bits & 0xFFFFFFFF )
				);

			Value /= Scale;

			return true;
		}

		//
		// Read a floating point value clamped to a range of [Scale1, Scale2]
		// and packed into an arbitrary number of bits less than 64.
		//

		inline
		bool
		ReadFLOAT(
			__out float &Value,
			__in float Scale1,
			__in float Scale2,
			__in size_t NumBits = sizeof( float ) * 8
			)
		{
			if (NumBits == sizeof( float ) * 8)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			float Factor  = (Scale2 - Scale1) / static_cast< float >( (1 << NumBits) - 1 );

			Value         = static_cast< float >( static_cast< double >( Bits ) * Factor );
			Value        += Scale1;

			return true;
		}
		 
		inline
		bool
		ReadDOUBLE(
			__out double &Value,
			__in size_t NumBits = sizeof( double ) * 8,
			__in double Scale = 1.0
			)
		{
			if ((NumBits == sizeof( double ) * 8) && (Scale == 1.0))
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value = static_cast< double >( Bits );

			Value /= Scale;

			return true;
		}

		inline
		bool
		ReadDOUBLE(
			__out double &Value,
			__in double Scale1,
			__in double Scale2,
			__in size_t NumBits = sizeof( float ) * 8
			)
		{
			if (NumBits == sizeof( float ) * 8)
				return m_ByteStream.GetField( Value );

			unsigned __int64 Bits;

			if (!ReadUnsigned( Bits, NumBits ))
				return false;

			Value  = (Scale2 - Scale1) / static_cast< double >( (1 << NumBits) - 1);
			Value  = static_cast< double >( static_cast< double >( Bits ) * Value );
			Value += Scale1;

			return true;
		}

		inline
		bool
		ReadOBJECTID(
			__out OBJECTID &ObjectId
			)
		{
			return m_ByteStream.GetField( ObjectId );
		}

		inline
		const void *
		ReadVOIDPtr(
			__in size_t FieldLength
			)
		{
			const void *DataPtr;

			if (!m_ByteStream.GetDataPtr( FieldLength, &DataPtr ))
				return NULL;

			return DataPtr;
		}

		inline
		bool
		ReadVector3(
			__out Vector3 &Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			)
		{
			if (!ReadFLOAT( Value.x, NumBits, Scale ))
				return false;

			if (!ReadFLOAT( Value.y, NumBits, Scale ))
				return false;

			if (!ReadFLOAT( Value.z, NumBits, Scale ))
				return false;

			return true;
		}

		inline
		bool
		ReadVector3(
			__out Vector3 &Value,
			__in float Scale1,
			__in float Scale2,
			__in size_t NumBits = sizeof( float ) * 8
			)
		{
			if (!ReadFLOAT( Value.x, Scale1, Scale2, NumBits ))
				return false;

			if (!ReadFLOAT( Value.y, Scale1, Scale2, NumBits ))
				return false;

			if (!ReadFLOAT( Value.z, Scale1, Scale2, NumBits ))
				return false;

			return true;
		}

		inline
		bool
		ReadVector2(
			__out Vector2 &Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			)
		{
			if (!ReadFLOAT( Value.x, NumBits, Scale ))
				return false;

			if (!ReadFLOAT( Value.y, NumBits, Scale ))
				return false;

			return true;
		}

		inline
		bool
		ReadVector2(
			__out Vector2 &Value,
			__in float Scale1,
			__in float Scale2,
			__in size_t NumBits = sizeof( float ) * 8
			)
		{
			if (!ReadFLOAT( Value.x, Scale1, Scale2, NumBits ))
				return false;

			if (!ReadFLOAT( Value.y, Scale1, Scale2, NumBits ))
				return false;

			return true;
		}

		inline
		bool
		ReadColor(
			__out NWNCOLOR &Value
			)
		{
			unsigned char v;

			if (!ReadBYTE( v ))
				return false;

			Value.r = (float) (((float) v) * (1.0/255.0));

			if (!ReadBYTE( v ))
				return false;

			Value.g = (float) (((float) v) * (1.0/255.0));

			if (!ReadBYTE( v ))
				return false;

			Value.b = (float) (((float) v) * (1.0/255.0));

			if (!ReadBYTE( v ))
				return false;

			Value.a = (float) (((float) v) * (1.0/255.0));

			return true;
		}

		/*
		inline
		bool
		ReadNWN2_DataElement(
			__out NWN2_DataElement & Element,
			__in bool Server
			)
		{
			int Count;

			Element.Bools.clear( );
			Element.Ints.clear( );
			Element.Floats.clear( );
			Element.StrRefs.clear( );
			Element.Strings.clear( );
			Element.LocStrings.clear( );
			Element.ObjectIds.clear( );

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				bool v;

				if (!ReadBOOL( v ))
					return false;

				Element.Bools.push_back( v );
			}

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				int v;

				if (!ReadINT( v ))
					return false;

				Element.Ints.push_back( v );
			}

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				float v;

				if (!ReadFLOAT( v ))
					return false;

				Element.Floats.push_back( v );
			}

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				int v;

				if (!ReadINT( v ))
					return false;

				Element.StrRefs.push_back( (unsigned long) v );
			}

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				std::string v;

				if (!ReadCExoString( v ))
					return false;

				Element.Strings.push_back( v );
			}

			if (Server)
			{
				if (!ReadINT( Count ))
					return false;

				while (Count--)
				{
					NWN::ExoLocString v;

					if (!ReadCExoLocString( v ))
						return false;

					Element.LocStrings.push_back( v );
				}
			}

			if (!ReadINT( Count ))
				return false;

			while (Count--)
			{
				OBJECTID v;

				if (!ReadOBJECTID( v ))
					return false;

				Element.ObjectIds.push_back( v );
			}

			return true;
		}*/

		inline
		bool
		ReadBits(
			__out unsigned __int64 &Value,
			__in size_t Bits
			)
		{
			return m_BitStream.GetFieldBits( Bits, Value );
		}

		inline
		bool
		AtEndOfStream(
			) const
		{
			return ((m_ByteStream.AtEndOfStream( )) &&
				(m_BitStream.AtEndOfStream( )));
		}

		inline
		size_t
		GetBytesRemaining(
			) const
		{
			return (m_ByteStream.GetBytesRemaining( ));
		}

		//
		// Debugging use only routine to get the current byte remaining count
		// for the bit stream.  The current byte being parsed is included in
		// the count should there be any legal bits left within it.
		//

		inline
		size_t
		GetBitStreamBytesRemaining(
			) const
		{
			return (m_BitStream.GetBytesRemaining( ));
		}

		//
		// Debugging use only routine to get the current extraction position in
		// the byte stream.
		//

		inline
		size_t
		GetBytePos(
			) const
		{
			return m_ByteStream.GetBytePos( );
		}

		//
		// Debugging use only routine to get a pointer into the raw byte data
		// stream.
		//

		inline
		const unsigned char *
		GetByteStreamBaseDataPointer(
			) const
		{
			return m_ByteStream.GetBaseDataPointer( );
		}

		//
		// Debugging use only routine to get a pointer into the raw bit data
		// stream.
		//

		inline
		const unsigned char *
		GetBitStreamBaseDataPointer(
			) const
		{
			return m_BitStream.GetBaseDataPointer( );
		}

		//
		// Debugging use only routine to get the current bit position in the
		// bit stream.
		//

		inline
		size_t
		GetBitPos(
			) const
		{
			return m_BitStream.GetBitPos( );
		}

		//
		// Debugging use only routine to get the highest valid bit number in
		// the last byte of the bit stream.
		//

		inline
		size_t
		GetHighestValidBitPos(
			) const
		{
			return m_BitStream.GetHighestValidBitPos( );
		}

		inline
		void
		SetHighestValidBitPos(
			__in size_t HighestValidBitPos
			)
		{
			return m_BitStream.SetHighestValidBitPos( HighestValidBitPos );
		}

	private:

		bool
		ReadSigned(
			__out unsigned __int64 &FieldBits,
			__in size_t NumBits
			);

		bool
		ReadUnsigned(
			__out unsigned __int64 &FieldBits,
			__in size_t NumBits
			);

		swutil::BufferParser m_ByteStream;
		swutil::BufferParser m_BitStream;

	};

	class ExoBuildBuffer
	{

	public:

		explicit ExoBuildBuffer(
			__in size_t ByteDataSizeHint = 32,
			__in size_t BitDataSizeHint = 1
			) throw( std::exception );

		~ExoBuildBuffer( );

		void
		WriteCExoString(
			__in const std::string &String,
			__in size_t NumBits = 32
			) throw( std::exception );

		void
		WriteCExoLocString(
			__in const std::string &String
			) throw( std::exception );

		void
		WriteCExoLocString(
			__in const NWN::ExoLocString &String
			) throw( std::exception );

		void
		WriteSmallString(
			__in const std::string &String,
			__in size_t NumBits = 8
			) throw( std::exception );

		inline
		void
		WriteResRef16(
			__in const ResRef16 &Ref,
			__in size_t NumBytes = sizeof( ResRef16 )
			) throw( std::exception )
		{
			m_ByteStream.AddData( NumBytes, &Ref );
		}

		inline
		void
		WriteResRef32(
			__in const ResRef32 &Ref,
			__in size_t NumBytes = sizeof( ResRef32 )
			) throw( std::exception )
		{
			m_ByteStream.AddData( NumBytes, &Ref );
		}

		inline
		void
		WriteBOOL(
			__in bool Value
			) throw( std::exception )
		{
			m_BitStream.AddFieldBit( Value );
		}

		inline
		void
		WriteCHAR(
			__in signed char Value,
			__in size_t NumBits = 8
			) throw( std::exception )
		{
			if (NumBits == 8)
				return m_ByteStream.AddField( Value );

			return WriteSigned(
				static_cast< signed __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteSHORT(
			__in signed short Value,
			__in size_t NumBits = 16
			) throw( std::exception )
		{
			if (NumBits == 16)
				return m_ByteStream.AddField( Value );

			return WriteSigned(
				static_cast< signed __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteINT(
			__in signed int Value,
			__in size_t NumBits = 32
			) throw( std::exception )
		{
			if (NumBits == 32)
				return m_ByteStream.AddField( Value );

			return WriteSigned(
				static_cast< signed __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteINT64(
			__in signed __int64 Value,
			__in size_t NumBits = 64
			) throw( std::exception )
		{
			if (NumBits == 64)
				return m_ByteStream.AddField( Value );

			return WriteSigned(
				static_cast< signed __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteBYTE(
			__in unsigned char Value,
			__in size_t NumBits = 8
			) throw( std::exception )
		{
			if (NumBits == 8)
				return m_ByteStream.AddField( Value );

			return WriteUnsigned(
				static_cast< unsigned __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteWORD(
			__in unsigned short Value,
			__in size_t NumBits = 16
			) throw( std::exception )
		{
			if (NumBits == 16)
				return m_ByteStream.AddField( Value );

			return WriteUnsigned(
				static_cast< unsigned __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteDWORD(
			__in unsigned long Value,
			__in size_t NumBits = 32
			) throw( std::exception )
		{
			if (NumBits == 32)
				return m_ByteStream.AddField( Value );

			return WriteUnsigned(
				static_cast< unsigned __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteDWORD64(
			__in unsigned __int64 Value,
			__in size_t NumBits = 64
			) throw( std::exception )
		{
			if (NumBits == 64)
				return m_ByteStream.AddField( Value );

			return WriteUnsigned(
				static_cast< unsigned __int64 >( Value ),
				NumBits
				);
		}

		inline
		void
		WriteFLOAT(
			__in float Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			) throw( std::exception )
		{
			if ((NumBits == sizeof( float ) * 8) && (Scale == 1.0))
				return m_ByteStream.AddField( Value );

			return WriteUnsigned(
				static_cast< unsigned __int64 >( Value * Scale ),
				NumBits
				);
		}

		//
		// Write a floating point value camped to a range of [Scale1, Scale2]
		// and packed into an arbitrary number of bits less than 64.
		//

		inline
		void
		WriteFLOAT(
			__in float Value,
			__in float Scale1,
			__in float Scale2,
			__in size_t NumBits = sizeof( float ) * 8
			) throw( std::exception )
		{
			if (NumBits == sizeof( float ) * 8)
				return m_ByteStream.AddField( Value );

			float Factor             = (Scale2 - Scale1) / static_cast< float >( (1 << NumBits) - 1 );
			float Scaled             = (Value - Scale1) / Factor;
			signed int    ScaledBits = static_cast< int >( Scaled );
			signed int   OScaledBits = ScaledBits;
			float         ScaledF    = static_cast< float >( ScaledBits );

			if (ScaledBits < 0)
				ScaledF += 4.294967296e9;

			ScaledF    *= Factor;
			ScaledBits += 1;

			ScaledF    += Scale1;
			ScaledF     = Value - ScaledF;

			float ScaledF2 = static_cast< float >( ScaledBits );

			if (ScaledBits < 0)
				ScaledF2 += 0.05f;

			Factor *= ScaledF2;
			Scale1 += Factor;
			Scale1 -= Value;

			if (ScaledF > Scale1)
			{
				return WriteUnsigned(
					static_cast< unsigned __int64 >( ScaledBits ),
					NumBits);
			}
			else
			{
				return WriteUnsigned(
					static_cast< unsigned __int64 >( OScaledBits ),
					NumBits);
			}
		}

		inline
		void
		WriteOBJECTID(
			__in OBJECTID ObjectId
			) throw( std::exception )
		{
			m_ByteStream.AddField( ObjectId );
		}

		inline
		void
		WriteVOIDPtr(
			__in_bcount( Length ) const void *Data,
			__in size_t Length
			) throw( std::exception )
		{
			m_ByteStream.AddData( Length, Data );
		}

		inline
		void
		WriteVector3(
			__in const Vector3 &Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			) throw( std::exception )
		{
			WriteFLOAT( Value.x, NumBits, Scale );
			WriteFLOAT( Value.y, NumBits, Scale );
			WriteFLOAT( Value.z, NumBits, Scale );
		}

		inline
		void
		WriteVector3(
			__in const Vector3 &Value,
			__in size_t NumBits,
			__in float Scale1,
			__in float Scale2
			) throw( std::exception )
		{
			WriteFLOAT( Value.x, Scale1, Scale2, NumBits );
			WriteFLOAT( Value.y, Scale1, Scale2, NumBits );
			WriteFLOAT( Value.z, Scale1, Scale2, NumBits );
		}

		inline
		void
		WriteVector2(
			__in const Vector2 &Value,
			__in size_t NumBits = sizeof( float ) * 8,
			__in float Scale = 1.0
			) throw( std::exception )
		{
			WriteFLOAT( Value.x, NumBits, Scale );
			WriteFLOAT( Value.y, NumBits, Scale );
		}

		inline
		void
		WriteColor(
			__in const NWNCOLOR &Value
			) throw( std::exception )
		{
			WriteBYTE( (unsigned char) (Value.r * 255.0) );
			WriteBYTE( (unsigned char) (Value.g * 255.0) );
			WriteBYTE( (unsigned char) (Value.b * 255.0) );
			WriteBYTE( (unsigned char) (Value.a * 255.0) );
		}

		inline
		void
		WriteNWN2_DataElement(
			__in const NWN2_DataElement & Element,
			__in bool Server
			) throw( std::exception )
		{
			WriteINT( (int) Element.Bools.size( ) );

			for (std::vector< bool >::const_iterator it = Element.Bools.begin( );
			     it != Element.Bools.end( );
			     ++it)
			{
				WriteBOOL( *it );
			}

			WriteINT( (int) Element.Ints.size( ) );

			for (std::vector< int >::const_iterator it = Element.Ints.begin( );
			     it != Element.Ints.end( );
			     ++it)
			{
				WriteINT( *it );
			}

			WriteINT( (int) Element.Floats.size( ) );

			for (std::vector< float >::const_iterator it = Element.Floats.begin( );
			     it != Element.Floats.end( );
			     ++it)
			{
				WriteFLOAT( *it );
			}

			WriteINT( (int) Element.StrRefs.size( ) );

			for (std::vector< unsigned long >::const_iterator it = Element.StrRefs.begin( );
			     it != Element.StrRefs.end( );
			     ++it)
			{
				WriteINT( (int) *it );
			}

			WriteINT( (int) Element.Strings.size( ) );

			for (std::vector< std::string >::const_iterator it = Element.Strings.begin( );
			     it != Element.Strings.end( );
			     ++it)
			{
				WriteCExoString( *it );
			}

			if (Server)
			{
				WriteINT( (int) Element.LocStrings.size( ) );

				for (std::vector< ExoLocString >::const_iterator it = Element.LocStrings.begin( );
					 it != Element.LocStrings.end( );
					 ++it)
				{
					WriteCExoLocString( *it );
				}
			}

			WriteINT( (int) Element.ObjectIds.size( ) );

			for (std::vector< OBJECTID >::const_iterator it = Element.ObjectIds.begin( );
				it != Element.ObjectIds.end( );
			    ++it)
			{
				WriteOBJECTID( *it );
			}
		}

		inline
		void
		WriteBits(
			__in unsigned __int64 Value,
			__in size_t Bits
			) throw( std::exception )
		{
			m_BitStream.AddFieldBits( Bits, Value );
		}

		inline
		void
		GetBuffer(
			__out unsigned char *&ByteBuffer,
			__out size_t &ByteBufferLength,
			__out unsigned char *&BitBuffer,
			__out size_t &BitBufferLength
			)
		{
			m_ByteStream.GetBuffer( ByteBuffer, ByteBufferLength );
			m_BitStream.GetBuffer( BitBuffer, BitBufferLength );

			//
			// If we have a bit stream, then let's write the highest valid bit
			// position into the buffer.  There are guaranteed to always be at
			// least three bits in the bit stream, which delimit the maximum
			// bit offset in the last byte of the bit stream.
			//

			if (BitBufferLength > 0)
			{
				size_t HighestValidBitPos = m_BitStream.GetBitPos( );

				BitBuffer[ 0 ] = (BitBuffer[ 0 ] & 0x1F) |
					static_cast< unsigned char >( (HighestValidBitPos << 5) & 0xFF );
			}
		}

		//
		// Perform a fast check as to whether any update data was written for
		// this buffer or not.
		//

		inline
		bool
		GetIsDataWritten(
			)
		{
			unsigned char * Buf;
			size_t          Length;

			m_ByteStream.GetBuffer( Buf, Length );

			if (Length != 0)
				return true;

			//
			// Now check if the bit stream is empty.  Discount the first three
			// bits we reserved space for (for the highest valid bit position)
			// as they are not really user data.
			//

			m_BitStream.GetBuffer( Buf, Length );

			if (Length > 1)
				return true;

			if (m_BitStream.GetBitPos( ) > 3)
				return true;

			return false;
		}

		//
		// Return the total size of the message payload being constructed, so
		// far.
		//
			
		inline
		size_t
		GetMessagePayloadSize(
			)
		{
			unsigned char * Buf;
			size_t          Length;
			size_t          TotalLength;

			m_ByteStream.GetBuffer( Buf, Length );

			TotalLength = Length;

			m_BitStream.GetBuffer( Buf, Length );

			TotalLength += Length;

			return TotalLength;
		}

	private:

		void
		WriteSigned(
			__in signed __int64 FieldBits,
			__in size_t NumBits
			) throw( std::exception );

		void
		WriteUnsigned(
			__in unsigned __int64 FieldBits,
			__in size_t NumBits
			) throw( std::exception );

		swutil::BufferBuilder m_ByteStream;
		swutil::BufferBuilder m_BitStream;

	};
}

#endif


#ifdef _MSC_VER

#pragma warning(pop)

#endif
