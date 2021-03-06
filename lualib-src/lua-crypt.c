#include <lua.h>
#include <lauxlib.h>

#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SMALL_CHUNK 256

/* the eight DES S-boxes */

static uint32_t SB1[64] = {
	0x01010400, 0x00000000, 0x00010000, 0x01010404,
	0x01010004, 0x00010404, 0x00000004, 0x00010000,
	0x00000400, 0x01010400, 0x01010404, 0x00000400,
	0x01000404, 0x01010004, 0x01000000, 0x00000004,
	0x00000404, 0x01000400, 0x01000400, 0x00010400,
	0x00010400, 0x01010000, 0x01010000, 0x01000404,
	0x00010004, 0x01000004, 0x01000004, 0x00010004,
	0x00000000, 0x00000404, 0x00010404, 0x01000000,
	0x00010000, 0x01010404, 0x00000004, 0x01010000,
	0x01010400, 0x01000000, 0x01000000, 0x00000400,
	0x01010004, 0x00010000, 0x00010400, 0x01000004,
	0x00000400, 0x00000004, 0x01000404, 0x00010404,
	0x01010404, 0x00010004, 0x01010000, 0x01000404,
	0x01000004, 0x00000404, 0x00010404, 0x01010400,
	0x00000404, 0x01000400, 0x01000400, 0x00000000,
	0x00010004, 0x00010400, 0x00000000, 0x01010004
};

static uint32_t SB2[64] = {
	0x80108020, 0x80008000, 0x00008000, 0x00108020,
	0x00100000, 0x00000020, 0x80100020, 0x80008020,
	0x80000020, 0x80108020, 0x80108000, 0x80000000,
	0x80008000, 0x00100000, 0x00000020, 0x80100020,
	0x00108000, 0x00100020, 0x80008020, 0x00000000,
	0x80000000, 0x00008000, 0x00108020, 0x80100000,
	0x00100020, 0x80000020, 0x00000000, 0x00108000,
	0x00008020, 0x80108000, 0x80100000, 0x00008020,
	0x00000000, 0x00108020, 0x80100020, 0x00100000,
	0x80008020, 0x80100000, 0x80108000, 0x00008000,
	0x80100000, 0x80008000, 0x00000020, 0x80108020,
	0x00108020, 0x00000020, 0x00008000, 0x80000000,
	0x00008020, 0x80108000, 0x00100000, 0x80000020,
	0x00100020, 0x80008020, 0x80000020, 0x00100020,
	0x00108000, 0x00000000, 0x80008000, 0x00008020,
	0x80000000, 0x80100020, 0x80108020, 0x00108000
};

static uint32_t SB3[64] = {
	0x00000208, 0x08020200, 0x00000000, 0x08020008,
	0x08000200, 0x00000000, 0x00020208, 0x08000200,
	0x00020008, 0x08000008, 0x08000008, 0x00020000,
	0x08020208, 0x00020008, 0x08020000, 0x00000208,
	0x08000000, 0x00000008, 0x08020200, 0x00000200,
	0x00020200, 0x08020000, 0x08020008, 0x00020208,
	0x08000208, 0x00020200, 0x00020000, 0x08000208,
	0x00000008, 0x08020208, 0x00000200, 0x08000000,
	0x08020200, 0x08000000, 0x00020008, 0x00000208,
	0x00020000, 0x08020200, 0x08000200, 0x00000000,
	0x00000200, 0x00020008, 0x08020208, 0x08000200,
	0x08000008, 0x00000200, 0x00000000, 0x08020008,
	0x08000208, 0x00020000, 0x08000000, 0x08020208,
	0x00000008, 0x00020208, 0x00020200, 0x08000008,
	0x08020000, 0x08000208, 0x00000208, 0x08020000,
	0x00020208, 0x00000008, 0x08020008, 0x00020200
};

static uint32_t SB4[64] = {
	0x00802001, 0x00002081, 0x00002081, 0x00000080,
	0x00802080, 0x00800081, 0x00800001, 0x00002001,
	0x00000000, 0x00802000, 0x00802000, 0x00802081,
	0x00000081, 0x00000000, 0x00800080, 0x00800001,
	0x00000001, 0x00002000, 0x00800000, 0x00802001,
	0x00000080, 0x00800000, 0x00002001, 0x00002080,
	0x00800081, 0x00000001, 0x00002080, 0x00800080,
	0x00002000, 0x00802080, 0x00802081, 0x00000081,
	0x00800080, 0x00800001, 0x00802000, 0x00802081,
	0x00000081, 0x00000000, 0x00000000, 0x00802000,
	0x00002080, 0x00800080, 0x00800081, 0x00000001,
	0x00802001, 0x00002081, 0x00002081, 0x00000080,
	0x00802081, 0x00000081, 0x00000001, 0x00002000,
	0x00800001, 0x00002001, 0x00802080, 0x00800081,
	0x00002001, 0x00002080, 0x00800000, 0x00802001,
	0x00000080, 0x00800000, 0x00002000, 0x00802080
};

static uint32_t SB5[64] = {
	0x00000100, 0x02080100, 0x02080000, 0x42000100,
	0x00080000, 0x00000100, 0x40000000, 0x02080000,
	0x40080100, 0x00080000, 0x02000100, 0x40080100,
	0x42000100, 0x42080000, 0x00080100, 0x40000000,
	0x02000000, 0x40080000, 0x40080000, 0x00000000,
	0x40000100, 0x42080100, 0x42080100, 0x02000100,
	0x42080000, 0x40000100, 0x00000000, 0x42000000,
	0x02080100, 0x02000000, 0x42000000, 0x00080100,
	0x00080000, 0x42000100, 0x00000100, 0x02000000,
	0x40000000, 0x02080000, 0x42000100, 0x40080100,
	0x02000100, 0x40000000, 0x42080000, 0x02080100,
	0x40080100, 0x00000100, 0x02000000, 0x42080000,
	0x42080100, 0x00080100, 0x42000000, 0x42080100,
	0x02080000, 0x00000000, 0x40080000, 0x42000000,
	0x00080100, 0x02000100, 0x40000100, 0x00080000,
	0x00000000, 0x40080000, 0x02080100, 0x40000100
};

static uint32_t SB6[64] = {
	0x20000010, 0x20400000, 0x00004000, 0x20404010,
	0x20400000, 0x00000010, 0x20404010, 0x00400000,
	0x20004000, 0x00404010, 0x00400000, 0x20000010,
	0x00400010, 0x20004000, 0x20000000, 0x00004010,
	0x00000000, 0x00400010, 0x20004010, 0x00004000,
	0x00404000, 0x20004010, 0x00000010, 0x20400010,
	0x20400010, 0x00000000, 0x00404010, 0x20404000,
	0x00004010, 0x00404000, 0x20404000, 0x20000000,
	0x20004000, 0x00000010, 0x20400010, 0x00404000,
	0x20404010, 0x00400000, 0x00004010, 0x20000010,
	0x00400000, 0x20004000, 0x20000000, 0x00004010,
	0x20000010, 0x20404010, 0x00404000, 0x20400000,
	0x00404010, 0x20404000, 0x00000000, 0x20400010,
	0x00000010, 0x00004000, 0x20400000, 0x00404010,
	0x00004000, 0x00400010, 0x20004010, 0x00000000,
	0x20404000, 0x20000000, 0x00400010, 0x20004010
};

static uint32_t SB7[64] = {
	0x00200000, 0x04200002, 0x04000802, 0x00000000,
	0x00000800, 0x04000802, 0x00200802, 0x04200800,
	0x04200802, 0x00200000, 0x00000000, 0x04000002,
	0x00000002, 0x04000000, 0x04200002, 0x00000802,
	0x04000800, 0x00200802, 0x00200002, 0x04000800,
	0x04000002, 0x04200000, 0x04200800, 0x00200002,
	0x04200000, 0x00000800, 0x00000802, 0x04200802,
	0x00200800, 0x00000002, 0x04000000, 0x00200800,
	0x04000000, 0x00200800, 0x00200000, 0x04000802,
	0x04000802, 0x04200002, 0x04200002, 0x00000002,
	0x00200002, 0x04000000, 0x04000800, 0x00200000,
	0x04200800, 0x00000802, 0x00200802, 0x04200800,
	0x00000802, 0x04000002, 0x04200802, 0x04200000,
	0x00200800, 0x00000000, 0x00000002, 0x04200802,
	0x00000000, 0x00200802, 0x04200000, 0x00000800,
	0x04000002, 0x04000800, 0x00000800, 0x00200002
};

static uint32_t SB8[64] = {
	0x10001040, 0x00001000, 0x00040000, 0x10041040,
	0x10000000, 0x10001040, 0x00000040, 0x10000000,
	0x00040040, 0x10040000, 0x10041040, 0x00041000,
	0x10041000, 0x00041040, 0x00001000, 0x00000040,
	0x10040000, 0x10000040, 0x10001000, 0x00001040,
	0x00041000, 0x00040040, 0x10040040, 0x10041000,
	0x00001040, 0x00000000, 0x00000000, 0x10040040,
	0x10000040, 0x10001000, 0x00041040, 0x00040000,
	0x00041040, 0x00040000, 0x10041000, 0x00001000,
	0x00000040, 0x10040040, 0x00001000, 0x00041040,
	0x10001000, 0x00000040, 0x10000040, 0x10040000,
	0x10040040, 0x10000000, 0x00040000, 0x10001040,
	0x00000000, 0x10041040, 0x00040040, 0x10000040,
	0x10040000, 0x10001000, 0x10001040, 0x00000000,
	0x10041040, 0x00041000, 0x00041000, 0x00001040,
	0x00001040, 0x00040040, 0x10000000, 0x10041000
};

/* PC1: left and right halves bit-swap */

static uint32_t LHs[16] = {
	0x00000000, 0x00000001, 0x00000100, 0x00000101,
	0x00010000, 0x00010001, 0x00010100, 0x00010101,
	0x01000000, 0x01000001, 0x01000100, 0x01000101,
	0x01010000, 0x01010001, 0x01010100, 0x01010101
};

static uint32_t RHs[16] = {
	0x00000000, 0x01000000, 0x00010000, 0x01010000,
	0x00000100, 0x01000100, 0x00010100, 0x01010100,
	0x00000001, 0x01000001, 0x00010001, 0x01010001,
	0x00000101, 0x01000101, 0x00010101, 0x01010101,
};

/* platform-independant 32-bit integer manipulation macros */

#define GET_UINT32(n,b,i)					   \
{											   \
	(n) = ( (uint32_t) (b)[(i)	] << 24 )	   \
		| ( (uint32_t) (b)[(i) + 1] << 16 )	   \
		| ( (uint32_t) (b)[(i) + 2] <<  8 )	   \
		| ( (uint32_t) (b)[(i) + 3]	   );	  \
}

#define PUT_UINT32(n,b,i)					   \
{											   \
	(b)[(i)	] = (uint8_t) ( (n) >> 24 );	   \
	(b)[(i) + 1] = (uint8_t) ( (n) >> 16 );	   \
	(b)[(i) + 2] = (uint8_t) ( (n) >>  8 );	   \
	(b)[(i) + 3] = (uint8_t) ( (n)	   );	   \
}

/* Initial Permutation macro */

#define DES_IP(X,Y)											 \
{															   \
	T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
	T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
	T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
	T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
	Y = ((Y << 1) | (Y >> 31)) & 0xFFFFFFFF;					\
	T = (X ^ Y) & 0xAAAAAAAA; Y ^= T; X ^= T;				   \
	X = ((X << 1) | (X >> 31)) & 0xFFFFFFFF;					\
}

/* Final Permutation macro */

#define DES_FP(X,Y)											 \
{															   \
	X = ((X << 31) | (X >> 1)) & 0xFFFFFFFF;					\
	T = (X ^ Y) & 0xAAAAAAAA; X ^= T; Y ^= T;				   \
	Y = ((Y << 31) | (Y >> 1)) & 0xFFFFFFFF;					\
	T = ((Y >>  8) ^ X) & 0x00FF00FF; X ^= T; Y ^= (T <<  8);   \
	T = ((Y >>  2) ^ X) & 0x33333333; X ^= T; Y ^= (T <<  2);   \
	T = ((X >> 16) ^ Y) & 0x0000FFFF; Y ^= T; X ^= (T << 16);   \
	T = ((X >>  4) ^ Y) & 0x0F0F0F0F; Y ^= T; X ^= (T <<  4);   \
}

/* DES round macro */

#define DES_ROUND(X,Y)						  \
{											   \
	T = *SK++ ^ X;							  \
	Y ^= SB8[ (T	  ) & 0x3F ] ^			  \
		 SB6[ (T >>  8) & 0x3F ] ^			  \
		 SB4[ (T >> 16) & 0x3F ] ^			  \
		 SB2[ (T >> 24) & 0x3F ];			   \
												\
	T = *SK++ ^ ((X << 28) | (X >> 4));		 \
	Y ^= SB7[ (T	  ) & 0x3F ] ^			  \
		 SB5[ (T >>  8) & 0x3F ] ^			  \
		 SB3[ (T >> 16) & 0x3F ] ^			  \
		 SB1[ (T >> 24) & 0x3F ];			   \
}

/* DES key schedule */

static void 
des_main_ks( uint32_t SK[32], const uint8_t key[8] ) {
	int i;
	uint32_t X, Y, T;

	GET_UINT32( X, key, 0 );
	GET_UINT32( Y, key, 4 );

	/* Permuted Choice 1 */

	T =  ((Y >>  4) ^ X) & 0x0F0F0F0F;  X ^= T; Y ^= (T <<  4);
	T =  ((Y	  ) ^ X) & 0x10101010;  X ^= T; Y ^= (T	  );

	X =   (LHs[ (X	  ) & 0xF] << 3) | (LHs[ (X >>  8) & 0xF ] << 2)
		| (LHs[ (X >> 16) & 0xF] << 1) | (LHs[ (X >> 24) & 0xF ]	 )
		| (LHs[ (X >>  5) & 0xF] << 7) | (LHs[ (X >> 13) & 0xF ] << 6)
		| (LHs[ (X >> 21) & 0xF] << 5) | (LHs[ (X >> 29) & 0xF ] << 4);

	Y =   (RHs[ (Y >>  1) & 0xF] << 3) | (RHs[ (Y >>  9) & 0xF ] << 2)
		| (RHs[ (Y >> 17) & 0xF] << 1) | (RHs[ (Y >> 25) & 0xF ]	 )
		| (RHs[ (Y >>  4) & 0xF] << 7) | (RHs[ (Y >> 12) & 0xF ] << 6)
		| (RHs[ (Y >> 20) & 0xF] << 5) | (RHs[ (Y >> 28) & 0xF ] << 4);

	X &= 0x0FFFFFFF;
	Y &= 0x0FFFFFFF;

	/* calculate subkeys */

	for( i = 0; i < 16; i++ )
	{
		if( i < 2 || i == 8 || i == 15 )
		{
			X = ((X <<  1) | (X >> 27)) & 0x0FFFFFFF;
			Y = ((Y <<  1) | (Y >> 27)) & 0x0FFFFFFF;
		}
		else
		{
			X = ((X <<  2) | (X >> 26)) & 0x0FFFFFFF;
			Y = ((Y <<  2) | (Y >> 26)) & 0x0FFFFFFF;
		}

		*SK++ =   ((X <<  4) & 0x24000000) | ((X << 28) & 0x10000000)
				| ((X << 14) & 0x08000000) | ((X << 18) & 0x02080000)
				| ((X <<  6) & 0x01000000) | ((X <<  9) & 0x00200000)
				| ((X >>  1) & 0x00100000) | ((X << 10) & 0x00040000)
				| ((X <<  2) & 0x00020000) | ((X >> 10) & 0x00010000)
				| ((Y >> 13) & 0x00002000) | ((Y >>  4) & 0x00001000)
				| ((Y <<  6) & 0x00000800) | ((Y >>  1) & 0x00000400)
				| ((Y >> 14) & 0x00000200) | ((Y	  ) & 0x00000100)
				| ((Y >>  5) & 0x00000020) | ((Y >> 10) & 0x00000010)
				| ((Y >>  3) & 0x00000008) | ((Y >> 18) & 0x00000004)
				| ((Y >> 26) & 0x00000002) | ((Y >> 24) & 0x00000001);

		*SK++ =   ((X << 15) & 0x20000000) | ((X << 17) & 0x10000000)
				| ((X << 10) & 0x08000000) | ((X << 22) & 0x04000000)
				| ((X >>  2) & 0x02000000) | ((X <<  1) & 0x01000000)
				| ((X << 16) & 0x00200000) | ((X << 11) & 0x00100000)
				| ((X <<  3) & 0x00080000) | ((X >>  6) & 0x00040000)
				| ((X << 15) & 0x00020000) | ((X >>  4) & 0x00010000)
				| ((Y >>  2) & 0x00002000) | ((Y <<  8) & 0x00001000)
				| ((Y >> 14) & 0x00000808) | ((Y >>  9) & 0x00000400)
				| ((Y	  ) & 0x00000200) | ((Y <<  7) & 0x00000100)
				| ((Y >>  7) & 0x00000020) | ((Y >>  3) & 0x00000011)
				| ((Y <<  2) & 0x00000004) | ((Y >> 21) & 0x00000002);
	}
}

/* DES 64-bit block encryption/decryption */

static void 
des_crypt( const uint32_t SK[32], const uint8_t input[8], uint8_t output[8] ) {
	uint32_t X, Y, T;

	GET_UINT32( X, input, 0 );
	GET_UINT32( Y, input, 4 );

	DES_IP( X, Y );

	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );
	DES_ROUND( Y, X );  DES_ROUND( X, Y );

	DES_FP( Y, X );

	PUT_UINT32( Y, output, 0 );
	PUT_UINT32( X, output, 4 );
}

static int
lrandomkey(lua_State *L) {
	char tmp[8];
	int i;
	for (i=0;i<8;i++) {
		tmp[i] = random() & 0xff;
	}
	lua_pushlstring(L, tmp, 8);
	return 1;
}

static void
des_key(lua_State *L, uint32_t SK[32]) {
	size_t keysz = 0;
	const void * key = luaL_checklstring(L, 1, &keysz);
	if (keysz != 8) {
		luaL_error(L, "Invalid key size %d, need 8 bytes", (int)keysz);
	}
	des_main_ks(SK, key);
}

static int
ldesencode(lua_State *L) {
	uint32_t SK[32];
	des_key(L, SK);

	size_t textsz = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 2, &textsz);
	size_t chunksz = (textsz + 8) & ~7;
	uint8_t tmp[SMALL_CHUNK];
	uint8_t *buffer = tmp;
	if (chunksz > SMALL_CHUNK) {
		buffer = lua_newuserdata(L, chunksz);
	}
	int i;
	for (i=0;i<(int)textsz-7;i+=8) {
		des_crypt(SK, text+i, buffer+i);
	}
	int bytes = textsz - i;
	uint8_t tail[8];
	int j;
	for (j=0;j<8;j++) {
		if (j < bytes) {
			tail[j] = text[i+j];
		} else if (j==bytes) {
			tail[j] = 0x80;
		} else {
			tail[j] = 0;
		}
	}
	des_crypt(SK, tail, buffer+i);
	lua_pushlstring(L, (const char *)buffer, chunksz);

	return 1;
}

static int
ldesdecode(lua_State *L) {
	uint32_t ESK[32];
	des_key(L, ESK);
	uint32_t SK[32];
	int i;
	for( i = 0; i < 32; i += 2 ) {
		SK[i] = ESK[30 - i];
		SK[i + 1] = ESK[31 - i];
	}
	size_t textsz = 0;
	const uint8_t *text = (const uint8_t *)luaL_checklstring(L, 2, &textsz);
	if ((textsz & 7) || textsz == 0) {
		return luaL_error(L, "Invalid des crypt text length %d", (int)textsz);
	}
	uint8_t tmp[SMALL_CHUNK];
	uint8_t *buffer = tmp;
	if (textsz > SMALL_CHUNK) {
		buffer = lua_newuserdata(L, textsz);
	}
	for (i=0;i<textsz;i+=8) {
		des_crypt(SK, text+i, buffer+i);
	}
	int padding = 1;
	for (i=textsz-1;i>=textsz-8;i--) {
		if (buffer[i] == 0) {
			padding++;
		} else if (buffer[i] == 0x80) {
			break;
		} else {
			return luaL_error(L, "Invalid des crypt text");
		}
	}
	if (padding > 8) {
		return luaL_error(L, "Invalid des crypt text");
	}
	lua_pushlstring(L, (const char *)buffer, textsz - padding);
	return 1;
}


static void
Hash(const char * str, int sz, uint8_t key[8]) {
	uint32_t djb_hash = 5381L;
	uint32_t js_hash = 1315423911L;

	int i;
	for (i=0;i<sz;i++) {
		uint8_t c = (uint8_t)str[i];
		djb_hash += (djb_hash << 5) + c;
		js_hash ^= ((js_hash << 5) + c + (js_hash >> 2));
	}

	key[0] = djb_hash & 0xff;
	key[1] = (djb_hash >> 8) & 0xff;
	key[2] = (djb_hash >> 16) & 0xff;
	key[3] = (djb_hash >> 24) & 0xff;

	key[4] = js_hash & 0xff;
	key[5] = (js_hash >> 8) & 0xff;
	key[6] = (js_hash >> 16) & 0xff;
	key[7] = (js_hash >> 24) & 0xff;
}

static int
lhashkey(lua_State *L) {
	size_t sz = 0;
	const char * key = luaL_checklstring(L, 1, &sz);
	uint8_t realkey[8];
	Hash(key,(int)sz,realkey);
	lua_pushlstring(L, (const char *)realkey, 8);
	return 1;
}

static int
ltohex(lua_State *L) {
	static char hex[] = "0123456789abcdef";
	size_t sz = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	char tmp[SMALL_CHUNK];
	char *buffer = tmp;
	if (sz > SMALL_CHUNK/2) {
		buffer = lua_newuserdata(L, sz * 2);
	}
	int i;
	for (i=0;i<sz;i++) {
		buffer[i*2] = hex[text[i] >> 4];
		buffer[i*2+1] = hex[text[i] & 0xf];
	}
	lua_pushlstring(L, buffer, sz * 2);
	return 1;
}

#define HEX(v,c) { char tmp = (char) c; if (tmp >= '0' && tmp <= '9') { v = tmp-'0'; } else { v = tmp - 'a' + 10; } }

static int
lfromhex(lua_State *L) {
	size_t sz = 0;
	const char * text = luaL_checklstring(L, 1, &sz);
	if (sz & 2) {
		return luaL_error(L, "Invalid hex text size %d", (int)sz);
	}
	char tmp[SMALL_CHUNK];
	char *buffer = tmp;
	if (sz > SMALL_CHUNK*2) {
		buffer = lua_newuserdata(L, sz / 2);
	}
	int i;
	for (i=0;i<sz;i+=2) {
		uint8_t hi,low;
		HEX(hi, text[i]);
		HEX(low, text[i+1]);
		if (hi > 16 || low > 16) {
			return luaL_error(L, "Invalid hex text", text);
		}
		buffer[i/2] = hi<<4 | low;
	}
	lua_pushlstring(L, buffer, i/2);
	return 1;
}

// Constants are the integer part of the sines of integers (in radians) * 2^32.
const uint32_t k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
 
// r specifies the per-round shift amounts
const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
					  5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
					  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
					  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
static void
hmac(uint32_t x[2], uint32_t y[2], uint32_t result[2]) {
	uint32_t w[16];
	uint32_t a, b, c, d, f, g, temp;
	int i;
 
	a = 0x67452301u;
	b = 0xefcdab89u;
	c = 0x98badcfeu;
	d = 0x10325476u;

	for (i=0;i<16;i+=4) {
		w[i] = x[1];
		w[i+1] = x[0];
		w[i+2] = y[1];
		w[i+3] = y[0];
	}

	for(i = 0; i<64; i++) {
		if (i < 16) {
			f = (b & c) | ((~b) & d);
			g = i;
		} else if (i < 32) {
			f = (d & b) | ((~d) & c);
			g = (5*i + 1) % 16;
		} else if (i < 48) {
			f = b ^ c ^ d;
			g = (3*i + 5) % 16; 
		} else {
			f = c ^ (b | (~d));
			g = (7*i) % 16;
		}

		temp = d;
		d = c;
		c = b;
		b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
		a = temp;

	}

	result[0] = c^d;
	result[1] = a^b;
}

static void
read64(lua_State *L, uint32_t xx[2], uint32_t yy[2]) {
	size_t sz = 0;
	const uint8_t *x = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	if (sz != 8) {
		luaL_error(L, "Invalid uint64 x");
	}
	const uint8_t *y = (const uint8_t *)luaL_checklstring(L, 2, &sz);
	if (sz != 8) {
		luaL_error(L, "Invalid uint64 y");
	}
	xx[0] = x[0] | x[1]<<8 | x[2]<<16 | x[3]<<24;
	xx[1] = x[4] | x[5]<<8 | x[6]<<16 | x[7]<<24;
	yy[0] = y[0] | y[1]<<8 | y[2]<<16 | y[3]<<24;
	yy[1] = y[4] | y[5]<<8 | y[6]<<16 | y[7]<<24;
}

static int
pushqword(lua_State *L, uint32_t result[2]) {
	uint8_t tmp[8];
	tmp[0] = result[0] & 0xff;
	tmp[1] = (result[0] >> 8 )& 0xff;
	tmp[2] = (result[0] >> 16 )& 0xff;
	tmp[3] = (result[0] >> 24 )& 0xff;
	tmp[4] = result[1] & 0xff;
	tmp[5] = (result[1] >> 8 )& 0xff;
	tmp[6] = (result[1] >> 16 )& 0xff;
	tmp[7] = (result[1] >> 24 )& 0xff;

	lua_pushlstring(L, (const char *)tmp, 8);
	return 1;
}

static int
lhmac64(lua_State *L) {
	uint32_t x[2], y[2];
	read64(L, x, y);
	uint32_t result[2];
	hmac(x,y,result);
	return pushqword(L, result);
}

/*
	8bytes key
	string text
 */
static int
lhmac_hash(lua_State *L) {
	uint32_t key[2];
	size_t sz = 0;
	const uint8_t *x = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	if (sz != 8) {
		luaL_error(L, "Invalid uint64 key");
	}
	key[0] = x[0] | x[1]<<8 | x[2]<<16 | x[3]<<24;
	key[1] = x[4] | x[5]<<8 | x[6]<<16 | x[7]<<24;
	const char * text = luaL_checklstring(L, 2, &sz);
	uint8_t h[8];
	Hash(text,(int)sz,h);
	uint32_t htext[2];
	htext[0] = h[0] | h[1]<<8 | h[2]<<16 | h[3]<<24;
	htext[1] = h[4] | h[5]<<8 | h[6]<<16 | h[7]<<24;
	uint32_t result[2];
	hmac(htext,key,result);
	return pushqword(L, result);
}

// powmodp64 for DH-key exchange

// The biggest 64bit prime
#define P 0xffffffffffffffc5ull

static inline uint64_t
mul_mod_p(uint64_t a, uint64_t b) {
	uint64_t m = 0;
	while(b) {
		if(b&1) {
			uint64_t t = P-a;
			if ( m >= t) {
				m -= t;
			} else {
				m += a;
			}
		}
		if (a >= P - a) {
			a = a * 2 - P;
		} else {
			a = a * 2;
		}
		b>>=1;
	}
	return m;
}

static inline uint64_t
pow_mod_p(uint64_t a, uint64_t b) {
	if (b==1) {
		return a;
	}
	uint64_t t = pow_mod_p(a, b>>1);
	t = mul_mod_p(t,t);
	if (b % 2) {
		t = mul_mod_p(t, a);
	}
	return t;
}

// calc a^b % p
static uint64_t
powmodp(uint64_t a, uint64_t b) {
	if (a > P)
		a%=P;
	return pow_mod_p(a,b);
}

static void
push64(lua_State *L, uint64_t r) {
	uint8_t tmp[8];
	tmp[0] = r & 0xff;
	tmp[1] = (r >> 8 )& 0xff;
	tmp[2] = (r >> 16 )& 0xff;
	tmp[3] = (r >> 24 )& 0xff;
	tmp[4] = (r >> 32 )& 0xff;
	tmp[5] = (r >> 40 )& 0xff;
	tmp[6] = (r >> 48 )& 0xff;
	tmp[7] = (r >> 56 )& 0xff;

	lua_pushlstring(L, (const char *)tmp, 8);
}

static int
ldhsecret(lua_State *L) {
	uint32_t x[2], y[2];
	read64(L, x, y);
	uint64_t r = powmodp((uint64_t)x[0] | (uint64_t)x[1]<<32,
		(uint64_t)y[0] | (uint64_t)y[1]<<32);

	push64(L, r);

	return 1;
}

#define G 5

static int
ldhexchange(lua_State *L) {
	size_t sz = 0;
	const uint8_t *x = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	if (sz != 8) {
		luaL_error(L, "Invalid dh uint64 key");
	}
	uint32_t xx[2];
	xx[0] = x[0] | x[1]<<8 | x[2]<<16 | x[3]<<24;
	xx[1] = x[4] | x[5]<<8 | x[6]<<16 | x[7]<<24;

	uint64_t r = powmodp(5,	(uint64_t)xx[0] | (uint64_t)xx[1]<<32);
	push64(L, r);
	return 1;
}

// base64

static int
lb64encode(lua_State *L) {
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t sz = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	int encode_sz = (sz + 2)/3*4;
	char tmp[SMALL_CHUNK];
	char *buffer = tmp;
	if (encode_sz > SMALL_CHUNK) {
		buffer = lua_newuserdata(L, encode_sz);
	}
	int i,j;
	j=0;
	for (i=0;i<(int)sz-2;i+=3) {
		uint32_t v = text[i] << 16 | text[i+1] << 8 | text[i+2];
		buffer[j] = encoding[v >> 18];
		buffer[j+1] = encoding[(v >> 12) & 0x3f];
		buffer[j+2] = encoding[(v >> 6) & 0x3f];
		buffer[j+3] = encoding[(v) & 0x3f];
		j+=4;
	}
	int padding = sz-i;
	uint32_t v;
	switch(padding) {
	case 1 :
		v = text[i];
		buffer[j] = encoding[v >> 2];
		buffer[j+1] = encoding[(v & 3) << 4];
		buffer[j+2] = '=';
		buffer[j+3] = '=';
		break;
	case 2 :
		v = text[i] << 8 | text[i+1];
		buffer[j] = encoding[v >> 10];
		buffer[j+1] = encoding[(v >> 4) & 0x3f];
		buffer[j+2] = encoding[(v & 0xf) << 2];
		buffer[j+3] = '=';
		break;
	}
	lua_pushlstring(L, buffer, encode_sz);
	return 1;
}

static inline int
b64index(uint8_t c) {
	static const int decoding[] = {62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
	int decoding_size = sizeof(decoding)/sizeof(decoding[0]);
	if (c<43) {
		return -1;
	}
	c -= 43;
	if (c>=decoding_size)
		return -1;
	return decoding[c];
}

static int
lb64decode(lua_State *L) {
	size_t sz = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	int decode_sz = (sz+3)/4*3;
	char tmp[SMALL_CHUNK];
	char *buffer = tmp;
	if (decode_sz > SMALL_CHUNK) {
		buffer = lua_newuserdata(L, decode_sz);
	}
	int i,j;
	int output = 0;
	for (i=0;i<sz;) {
		int padding = 0;
		int c[4];
		for (j=0;j<4;) {
			if (i>=sz) {
				return luaL_error(L, "Invalid base64 text");
			}
			c[j] = b64index(text[i]);
			if (c[j] == -1) {
				++i;
				continue;
			}
			if (c[j] == -2) {
				++padding;
			}
			++i;
			++j;
		}
		uint32_t v;
		switch (padding) {
		case 0:
			v = (unsigned)c[0] << 18 | c[1] << 12 | c[2] << 6 | c[3];
			buffer[output] = v >> 16;
			buffer[output+1] = (v >> 8) & 0xff;
			buffer[output+2] = v & 0xff;
			output += 3;
			break;
		case 1:
			if (c[3] != -2 || (c[2] & 3)!=0) {
				return luaL_error(L, "Invalid base64 text");
			}
			v = (unsigned)c[0] << 10 | c[1] << 4 | c[2] >> 2 ;
			buffer[output] = v >> 8;
			buffer[output+1] = v & 0xff;
			output += 2;
			break;
		case 2:
			if (c[3] != -2 || c[2] != -2 || (c[1] & 0xf) !=0)  {
				return luaL_error(L, "Invalid base64 text");
			}
			v = (unsigned)c[0] << 2 | c[1] >> 4;
			buffer[output] = v;
			++ output;
			break;
		default:
			return luaL_error(L, "Invalid base64 text");
		}
	}
	lua_pushlstring(L, buffer, output);
	return 1;
}

//==========================================================================================
/*
*
*
*
*
*
*												AES
*
*
*
*
*/
//=============================================================================================
static int encrypt(unsigned char *input, int len, unsigned char *key, int klen);
static int decrypt(unsigned char *input, int len, unsigned char *key, int klen);
static int encryptCBC(unsigned char *input, int len, unsigned char *key, int klen, unsigned char *iv);
static int decryptCBC(unsigned char *input, int len, unsigned char *key, int klen, unsigned char *iv);
static void trimKey(char *key);
static unsigned char* encryptAES(unsigned char* input,
	int inputLen,
	unsigned char* key,
	int keyLen,
	int* outputLen,
	unsigned char* iv);

static unsigned char* decryptAES(unsigned char* input,
	int inputLen,
	unsigned char* key,
	int keyLen,
	int* outputLen,
	unsigned char* iv);

#define min(a, b)   ((a < b) ? a : b)

typedef unsigned char state_t[4][4];
typedef unsigned char row_t[4];

unsigned char Rcon[] = {
	0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
	0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
	0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a,
	0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39,
	0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25,
	0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a,
	0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08,
	0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
	0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6,
	0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
	0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61,
	0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc,
	0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01,
	0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b,
	0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e,
	0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
	0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4,
	0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
	0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8,
	0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20,
	0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d,
	0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35,
	0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
	0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
	0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d,
	0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
	0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c,
	0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63,
	0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa,
	0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd,
	0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66,
	0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb
};

unsigned char Sbox[] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
	0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
	0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
	0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
	0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
	0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
	0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
	0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
	0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
	0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
	0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
	0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
	0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
	0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
	0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
	0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
	0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

unsigned char InvSbox[] = {
	0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
	0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
	0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
	0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
	0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
	0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
	0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
	0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
	0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
	0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
	0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
	0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
	0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
	0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
	0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
	0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
	0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
	0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
	0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
	0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
	0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
	0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
	0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
	0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
	0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
	0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
	0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
	0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
	0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
	0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
	0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

unsigned char GFMul2[] = {
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e,
	0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
	0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e,
	0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
	0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e,
	0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
	0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e,
	0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
	0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e,
	0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
	0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae,
	0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
	0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce,
	0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
	0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee,
	0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,
	0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15,
	0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
	0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35,
	0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25,
	0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55,
	0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45,
	0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75,
	0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65,
	0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95,
	0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85,
	0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5,
	0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5,
	0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5,
	0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5,
	0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5,
	0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5
};

unsigned char GFMul4[] = {
	0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
	0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c,
	0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c,
	0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c,
	0x80, 0x84, 0x88, 0x8c, 0x90, 0x94, 0x98, 0x9c,
	0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc,
	0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd4, 0xd8, 0xdc,
	0xe0, 0xe4, 0xe8, 0xec, 0xf0, 0xf4, 0xf8, 0xfc,
	0x1b, 0x1f, 0x13, 0x17, 0x0b, 0x0f, 0x03, 0x07,
	0x3b, 0x3f, 0x33, 0x37, 0x2b, 0x2f, 0x23, 0x27,
	0x5b, 0x5f, 0x53, 0x57, 0x4b, 0x4f, 0x43, 0x47,
	0x7b, 0x7f, 0x73, 0x77, 0x6b, 0x6f, 0x63, 0x67,
	0x9b, 0x9f, 0x93, 0x97, 0x8b, 0x8f, 0x83, 0x87,
	0xbb, 0xbf, 0xb3, 0xb7, 0xab, 0xaf, 0xa3, 0xa7,
	0xdb, 0xdf, 0xd3, 0xd7, 0xcb, 0xcf, 0xc3, 0xc7,
	0xfb, 0xff, 0xf3, 0xf7, 0xeb, 0xef, 0xe3, 0xe7,
	0x36, 0x32, 0x3e, 0x3a, 0x26, 0x22, 0x2e, 0x2a,
	0x16, 0x12, 0x1e, 0x1a, 0x06, 0x02, 0x0e, 0x0a,
	0x76, 0x72, 0x7e, 0x7a, 0x66, 0x62, 0x6e, 0x6a,
	0x56, 0x52, 0x5e, 0x5a, 0x46, 0x42, 0x4e, 0x4a,
	0xb6, 0xb2, 0xbe, 0xba, 0xa6, 0xa2, 0xae, 0xaa,
	0x96, 0x92, 0x9e, 0x9a, 0x86, 0x82, 0x8e, 0x8a,
	0xf6, 0xf2, 0xfe, 0xfa, 0xe6, 0xe2, 0xee, 0xea,
	0xd6, 0xd2, 0xde, 0xda, 0xc6, 0xc2, 0xce, 0xca,
	0x2d, 0x29, 0x25, 0x21, 0x3d, 0x39, 0x35, 0x31,
	0x0d, 0x09, 0x05, 0x01, 0x1d, 0x19, 0x15, 0x11,
	0x6d, 0x69, 0x65, 0x61, 0x7d, 0x79, 0x75, 0x71,
	0x4d, 0x49, 0x45, 0x41, 0x5d, 0x59, 0x55, 0x51,
	0xad, 0xa9, 0xa5, 0xa1, 0xbd, 0xb9, 0xb5, 0xb1,
	0x8d, 0x89, 0x85, 0x81, 0x9d, 0x99, 0x95, 0x91,
	0xed, 0xe9, 0xe5, 0xe1, 0xfd, 0xf9, 0xf5, 0xf1,
	0xcd, 0xc9, 0xc5, 0xc1, 0xdd, 0xd9, 0xd5, 0xd1
};

unsigned char GFMul8[] = {
	0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
	0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
	0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8,
	0xc0, 0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xf0, 0xf8,
	0x1b, 0x13, 0x0b, 0x03, 0x3b, 0x33, 0x2b, 0x23,
	0x5b, 0x53, 0x4b, 0x43, 0x7b, 0x73, 0x6b, 0x63,
	0x9b, 0x93, 0x8b, 0x83, 0xbb, 0xb3, 0xab, 0xa3,
	0xdb, 0xd3, 0xcb, 0xc3, 0xfb, 0xf3, 0xeb, 0xe3,
	0x36, 0x3e, 0x26, 0x2e, 0x16, 0x1e, 0x06, 0x0e,
	0x76, 0x7e, 0x66, 0x6e, 0x56, 0x5e, 0x46, 0x4e,
	0xb6, 0xbe, 0xa6, 0xae, 0x96, 0x9e, 0x86, 0x8e,
	0xf6, 0xfe, 0xe6, 0xee, 0xd6, 0xde, 0xc6, 0xce,
	0x2d, 0x25, 0x3d, 0x35, 0x0d, 0x05, 0x1d, 0x15,
	0x6d, 0x65, 0x7d, 0x75, 0x4d, 0x45, 0x5d, 0x55,
	0xad, 0xa5, 0xbd, 0xb5, 0x8d, 0x85, 0x9d, 0x95,
	0xed, 0xe5, 0xfd, 0xf5, 0xcd, 0xc5, 0xdd, 0xd5,
	0x6c, 0x64, 0x7c, 0x74, 0x4c, 0x44, 0x5c, 0x54,
	0x2c, 0x24, 0x3c, 0x34, 0x0c, 0x04, 0x1c, 0x14,
	0xec, 0xe4, 0xfc, 0xf4, 0xcc, 0xc4, 0xdc, 0xd4,
	0xac, 0xa4, 0xbc, 0xb4, 0x8c, 0x84, 0x9c, 0x94,
	0x77, 0x7f, 0x67, 0x6f, 0x57, 0x5f, 0x47, 0x4f,
	0x37, 0x3f, 0x27, 0x2f, 0x17, 0x1f, 0x07, 0x0f,
	0xf7, 0xff, 0xe7, 0xef, 0xd7, 0xdf, 0xc7, 0xcf,
	0xb7, 0xbf, 0xa7, 0xaf, 0x97, 0x9f, 0x87, 0x8f,
	0x5a, 0x52, 0x4a, 0x42, 0x7a, 0x72, 0x6a, 0x62,
	0x1a, 0x12, 0x0a, 0x02, 0x3a, 0x32, 0x2a, 0x22,
	0xda, 0xd2, 0xca, 0xc2, 0xfa, 0xf2, 0xea, 0xe2,
	0x9a, 0x92, 0x8a, 0x82, 0xba, 0xb2, 0xaa, 0xa2,
	0x41, 0x49, 0x51, 0x59, 0x61, 0x69, 0x71, 0x79,
	0x01, 0x09, 0x11, 0x19, 0x21, 0x29, 0x31, 0x39,
	0xc1, 0xc9, 0xd1, 0xd9, 0xe1, 0xe9, 0xf1, 0xf9,
	0x81, 0x89, 0x91, 0x99, 0xa1, 0xa9, 0xb1, 0xb9
};

static unsigned char *SubWord(unsigned char *w)
{
	w[0] = Sbox[w[0]];
	w[1] = Sbox[w[1]];
	w[2] = Sbox[w[2]];
	w[3] = Sbox[w[3]];

	return w;
}

static unsigned char *RotWord(unsigned char *w)
{
	unsigned char c;
	c = w[0];

	w[0] = w[1];
	w[1] = w[2];
	w[2] = w[3];
	w[3] = c;

	return w;
}

static unsigned char *KeyExpansion(unsigned char *key, unsigned char *w, int klen)
{
	unsigned char temp[4];
	int i, j, i4, j4, r;

	r = 4 * (klen + 7);

	for (i = 0, j = 0; i < klen; i++, j += 4) {
		memcpy(&w[i * 4], &key[j], 4);
	}

	while (i < r) {
		memcpy(&temp, &w[(i - 1) * 4], 4);

		if (i % klen == 0) {
			SubWord(RotWord(temp));
			temp[0] ^= Rcon[i / klen];
		}
		else if (klen > 6 && i % klen == 4) {
			SubWord(temp);
		}
		j = i - klen;
		i4 = i * 4;
		j4 = j * 4;
		w[i4] = w[j4] ^ temp[0];
		w[i4 + 1] = w[j4 + 1] ^ temp[1];
		w[i4 + 2] = w[j4 + 2] ^ temp[2];
		w[i4 + 3] = w[j4 + 3] ^ temp[3];

		i++;
	}
	return w;
}

static void AddRoundKey(state_t *state, unsigned char *key, int round)
{
	int i, r = round * 16, n = 0;

	for (i = 0; i < 4; i++) {
		int j;
		for (j = 0; j < 4; j++) {
			(*state)[i][j] ^= key[r + n++];
		}
	}
}

static void SubShiftRows(state_t *state)
{
	unsigned char c, c1;

	(*state)[0][0] = Sbox[(*state)[0][0]];
	(*state)[1][0] = Sbox[(*state)[1][0]];
	(*state)[2][0] = Sbox[(*state)[2][0]];
	(*state)[3][0] = Sbox[(*state)[3][0]];

	c = Sbox[(*state)[0][1]];
	(*state)[0][1] = Sbox[(*state)[1][1]];
	(*state)[1][1] = Sbox[(*state)[2][1]];
	(*state)[2][1] = Sbox[(*state)[3][1]];
	(*state)[3][1] = c;

	c = Sbox[(*state)[0][2]];
	c1 = Sbox[(*state)[1][2]];
	(*state)[0][2] = Sbox[(*state)[2][2]];
	(*state)[1][2] = Sbox[(*state)[3][2]];
	(*state)[2][2] = c;
	(*state)[3][2] = c1;

	c = Sbox[(*state)[3][3]];
	(*state)[3][3] = Sbox[(*state)[2][3]];
	(*state)[2][3] = Sbox[(*state)[1][3]];
	(*state)[1][3] = Sbox[(*state)[0][3]];
	(*state)[0][3] = c;
}

static void SubInvShiftRows(state_t *state)
{
	unsigned char c, c1;

	(*state)[0][0] = InvSbox[(*state)[0][0]];
	(*state)[1][0] = InvSbox[(*state)[1][0]];
	(*state)[2][0] = InvSbox[(*state)[2][0]];
	(*state)[3][0] = InvSbox[(*state)[3][0]];

	c = InvSbox[(*state)[0][1]];
	(*state)[0][1] = InvSbox[(*state)[3][1]];
	(*state)[3][1] = InvSbox[(*state)[2][1]];
	(*state)[2][1] = InvSbox[(*state)[1][1]];
	(*state)[1][1] = c;

	c = InvSbox[(*state)[1][2]];
	c1 = InvSbox[(*state)[0][2]];
	(*state)[0][2] = InvSbox[(*state)[2][2]];
	(*state)[1][2] = InvSbox[(*state)[3][2]];
	(*state)[2][2] = c1;
	(*state)[3][2] = c;

	c = InvSbox[(*state)[1][3]];
	(*state)[1][3] = InvSbox[(*state)[2][3]];
	(*state)[2][3] = InvSbox[(*state)[3][3]];
	(*state)[3][3] = InvSbox[(*state)[0][3]];
	(*state)[0][3] = c;
}

static void MixColumns(state_t *state)
{
	int i;
	unsigned char ad, bc, abcd;
	row_t *rs;

	for (i = 0; i < 4; i++) {
		rs = &(*state)[i];
		ad = (*rs)[0] ^ (*rs)[3];
		bc = (*rs)[1] ^ (*rs)[2];
		abcd = ad ^ bc;

		(*rs)[0] ^= abcd ^ GFMul2[(*rs)[0] ^ (*rs)[1]];
		(*rs)[1] ^= abcd ^ GFMul2[bc];
		(*rs)[2] ^= abcd ^ GFMul2[(*rs)[2] ^ (*rs)[3]];
		(*rs)[3] ^= abcd ^ GFMul2[ad];
	}
}

static void InvMixColumns(state_t *state)
{
	int i;
	unsigned char ad, bc, p, q;
	row_t *rs;

	for (i = 0; i < 4; i++) {
		rs = &(*state)[i];
		ad = (*rs)[0] ^ (*rs)[3];
		bc = (*rs)[1] ^ (*rs)[2];
		q = ad ^ bc;
		q ^= GFMul8[q];
		p = q ^GFMul4[(*rs)[0] ^ (*rs)[2]];
		q = q ^GFMul4[(*rs)[1] ^ (*rs)[3]];

		(*rs)[0] ^= p ^ GFMul2[(*rs)[0] ^ (*rs)[1]];
		(*rs)[1] ^= q ^ GFMul2[bc];
		(*rs)[2] ^= p ^ GFMul2[(*rs)[2] ^ (*rs)[3]];
		(*rs)[3] ^= q ^ GFMul2[ad];
	}
}

static void encryptBlock(unsigned char *in, unsigned char *out,
	unsigned char *w, int Nr)
{
	int round, i, j, n = 0;
	state_t state;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			state[i][j] = in[n++];
		}
	}

	AddRoundKey(&state, w, 0);

	for (round = 1; round < Nr; round++) {
		SubShiftRows(&state);
		MixColumns(&state);
		AddRoundKey(&state, w, round);
	}

	SubShiftRows(&state);
	AddRoundKey(&state, w, Nr);

	for (i = 0, n = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out[n++] = state[i][j];
		}
	}

}

static void decryptBlock(unsigned char *in, unsigned char *out,
	unsigned char *w, int Nr)
{
	int round, i, j, n = 0;
	state_t state;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			state[i][j] = in[n++];
		}
	}

	AddRoundKey(&state, w, Nr);

	for (round = Nr - 1; round > 0; round--) {
		SubInvShiftRows(&state);
		AddRoundKey(&state, w, round);
		InvMixColumns(&state);
	}

	SubInvShiftRows(&state);
	AddRoundKey(&state, w, 0);

	for (i = 0, n = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out[n++] = state[i][j];
		}
	}
}

/*
* ECB encryption routine
* size of 'input' has to be multiple of 16
* 'input' contains ciphertext on exit
*/
static int encrypt(unsigned char *input, int len, unsigned char *key, int klen)
{
	int i;
	unsigned char w[32 * 15];
	unsigned char k[32] = { 0 };

	if (input == 0 || key == 0) {
		return 0;
	}

	memcpy(k, key, min(klen, 32));
	KeyExpansion(k, w, 8);

	for (i = 0; i < len; i += 16) {
		encryptBlock(&input[i], &input[i], w, 14);
	}

	return 1;
}

/*
* ECB decryption routine
* size of 'input' has to be multiple of 16
* 'input' contains cleartext on exit
*/
static int decrypt(unsigned char *input, int len, unsigned char *key, int klen)
{
	int i;
	unsigned char w[32 * 15] = { 0 };
	unsigned char k[32] = { 0 };

	if (input == 0 || key == 0) {
		return 0;
	}

	memcpy(k, key, min(klen, 32));
	KeyExpansion(k, w, 8);

	for (i = 0; i < len; i += 16) {
		decryptBlock(&input[i], &input[i], w, 14);
	}

	return 1;
}

/*
* CBC encryption routine
* size of 'input' has to be multiple of 16
* 'input' contains ciphertext on exit
* 'iv' must hold a 16 byte initialization vector
*/
static int encryptCBC(unsigned char *input, int len, unsigned char *key, int klen,unsigned char *iv)
{
	int i;
	unsigned char w[32 * 15];
	unsigned char k[32] = { 0 };
	unsigned char piv[16];

	if (input == 0 || key == 0 || iv == 0) {
		return 0;
	}

	memcpy(piv, iv, 16);

	memcpy(k, key, min(klen, 32));
	KeyExpansion(k, w, 8);

	for (i = 0; i < len; i += 16) {
		int n;
		for (n = 0; n < 16; n++) {
			input[i + n] ^= piv[n];
		}
		encryptBlock(&input[i], &input[i], w, 14);
		memcpy(piv, &input[i], 16);
	}

	return 1;
}

/*
* CBC decryption routine
* size of 'input' has to be multiple of 16
* 'input' contains cleartext on exit
* 'iv' must hold the same 16 byte initialization vector
* that has been used to encrypt the cleartext
*/
static int decryptCBC(unsigned char *input, int len, unsigned char *key, int klen,unsigned char *iv)
{
	int i;
	unsigned char w[32 * 15] = { 0 };
	unsigned char k[32] = { 0 };
	unsigned char piv[16];
	unsigned char tpiv[16];

	if (input == 0 || key == 0 || iv == 0) {
		return 0;
	}

	memcpy(piv, iv, 16);

	memcpy(k, key, min(klen, 32));
	KeyExpansion(k, w, 8);

	for (i = 0; i < len; i += 16) {
		int n;
		memcpy(tpiv, &input[i], 16);
		decryptBlock(&input[i], &input[i], w, 14);
		for (n = 0; n < 16; n++) {
			input[i + n] ^= piv[n];
		}
		memcpy(piv, tpiv, 16);
	}

	return 1;
}

/*
* key should not have and \r or \n
* character at the end to ensure compatibility
*/
static void trimKey(char *key)
{
	char *cp = key;
	char *ep = &key[strlen(key)];

	while (ep > cp) {
		ep--;
		if (*ep == 0x0d || *ep == 0x0a) {
			*ep = 0x00;
		}
		else {
			break;
		}
	}
}

static unsigned char* encryptAES(unsigned char* input, int inputLen, unsigned char* key, int keyLen, int* outputLen, unsigned char* iv)
{
	int lastn = inputLen & 0x0F;	// calculate how many bytes of final block are real data
	unsigned char fill_block[16];
	int i = 0;
	for (i = 0; i < 16; i++)
	{
		fill_block[i] = rand() & 0xFF;
	}
	fill_block[0] &= 0xF0;
	fill_block[0] |= lastn;

	int len = inputLen + 16;
	if (lastn)
	{
		len += (16 - lastn);
	}

	unsigned char* result = (unsigned char*)malloc(len);
	memset(result, 0, len);
	memcpy(result, input, inputLen);

	if (iv == NULL)
	{
		encrypt(result, inputLen, key, keyLen);
	}
	else
	{
		encryptCBC(result, inputLen, key, keyLen, iv);
	}

	memcpy(&result[len - 16], fill_block, 16);

	*outputLen = len;

	return result;
}

static unsigned char* decryptAES(unsigned char* input, int inputLen, unsigned char* key, int keyLen, int* outputLen, unsigned char* iv)
{
	// 解密数据长度不能小于16
	if (inputLen < 16)
	{
		return NULL;
	}

	int len = inputLen - 16;
	unsigned char fill_block[16];
	memcpy(fill_block, &input[len], 16);
	int lastn = fill_block[0] & 0x0F;

	unsigned char* result = (unsigned char*)malloc(len + 1);
	memset(result, 0, len + 1);
	memcpy(result, input, len);

	if (iv == NULL)
	{
		decrypt(result, len, key, keyLen);
	}
	else
	{
		decryptCBC(result, len, key, keyLen, iv);
	}

	if (lastn)
	{
		len -= (16 - lastn);
	}

	*outputLen = len;

	return result;
}

static int laesencode(lua_State *L) 
{
#if 0
	//size_t textsz = 0;
	//size_t keysz = 0;
	//const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &textsz);
	//const uint8_t * key = (const uint8_t *)luaL_checklstring(L, 2, &keysz);
	//int outlen = (textsz / 16 + 1) * 16;
	//unsigned char buf[outlen];
	//memset(buf, 0, sizeof(buf));
	//memcpy(buf, text, textsz);
	////printf("encode before:text=%s\n key=%s\n textlen=%d\n keylen=%d\n", buf, key, textsz, keysz);
	//trimKey(key);

	//unsigned char iv[] = {
	//	0x22, 0x10, 0x19, 0x64,
	//	0x10, 0x19, 0x64, 0x22,
	//	0x19, 0x64, 0x22, 0x10,
	//	0x64, 0x22, 0x10, 0x19
	//};

	//encrypt((unsigned char*)buf, sizeof(buf), (unsigned char*)key, keysz);

	////printf("encode after:text=%s\n textlen=%d\n", buf, sizeof(buf));
	//lua_pushlstring(L, (const char *)buf, sizeof(buf));
	//return 1;
#else
	size_t textsz = 0;
	size_t keysz = 0;
	size_t vecsz = 0;
	int outLen = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &textsz);
	const uint8_t * key = (const uint8_t *)luaL_checklstring(L, 2, &keysz);
	const uint8_t * vec = (const uint8_t *)luaL_checklstring(L, 3, &vecsz);

	unsigned char iv[] = 
	{
		0x22, 0x10, 0x19, 0x64,
		0x10, 0x19, 0x64, 0x22,
		0x19, 0x64, 0x22, 0x10,
		0x64, 0x22, 0x10, 0x19
	};

	unsigned char* buf;
	if (strlen(vec) != 16)
	{
		buf = encryptAES((unsigned char*)text, textsz, (unsigned char*)key, keysz, &outLen, NULL);
	}
	else
	{
		buf = encryptAES((unsigned char*)text, textsz, (unsigned char*)key, keysz, &outLen, vec);
	}

	//printf("encode :%s     \n size:%d\n", buf, outLen);
	lua_pushlstring(L, (const char *)buf, outLen);
	lua_pushinteger(L, outLen);
	free(buf);
	return 2;
#endif
}

static int laesdecode(lua_State *L) 
{
#if 0
	//size_t textsz = 0;
	//size_t keysz = 0;
	//const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &textsz);
	//const uint8_t * key = (const uint8_t *)luaL_checklstring(L, 2, &keysz);
	//unsigned char buf[textsz];
	//memset(buf, 0, sizeof(buf));
	//memcpy(buf, text, textsz);
	////printf("decode before:text=%s\n key=%s\n textlen=%d\n keylen=%d\n", buf, key, textsz, keysz);
	//trimKey(key);

	/*unsigned char iv[] = {
		0x22, 0x10, 0x19, 0x64,
		0x10, 0x19, 0x64, 0x22,
		0x19, 0x64, 0x22, 0x10,
		0x64, 0x22, 0x10, 0x19
	};*/

	//decrypt((unsigned char *)buf, sizeof(buf), (unsigned char *)key, keysz);

	////printf("decode after:text=%s\n textlen=%d\n", buf, strlen(buf));
	//lua_pushlstring(L, (const char *)buf, strlen(buf));
	//return 1;
#else
	size_t textsz = 0;
	size_t keysz = 0;
	size_t vecsz = 0;
	int outLen = 0;
	const uint8_t * text = (const uint8_t *)luaL_checklstring(L, 1, &textsz);
	const uint8_t * key = (const uint8_t *)luaL_checklstring(L, 2, &keysz);
	const uint8_t * vec = (const uint8_t *)luaL_checklstring(L, 3, &vecsz);

	unsigned char iv[] =
	{
		0x22, 0x10, 0x19, 0x64,
		0x10, 0x19, 0x64, 0x22,
		0x19, 0x64, 0x22, 0x10,
		0x64, 0x22, 0x10, 0x19
	};

	unsigned char* buf;
	if (strlen(vec) != 16)
	{
		buf = decryptAES((unsigned char*)text, textsz, (unsigned char*)key, keysz, &outLen, NULL);
	}
	else
	{
		buf = decryptAES((unsigned char*)text, textsz, (unsigned char*)key, keysz, &outLen, vec);
	}

	//printf("decode :%s     \n size:%d\n", buf, outLen);
	lua_pushlstring(L, (const char *)buf, outLen);
	free(buf);
	return 1;
#endif
}

// defined in lsha1.c
int lsha1(lua_State *L);
int lhmac_sha1(lua_State *L);

int
luaopen_crypt(lua_State *L) {
	luaL_checkversion(L);
	srandom(time(NULL));
	luaL_Reg l[] = {
		{ "hashkey", lhashkey },
		{ "randomkey", lrandomkey },
		{ "desencode", ldesencode },
		{ "desdecode", ldesdecode },
		{ "hexencode", ltohex },
		{ "hexdecode", lfromhex },
		{ "hmac64", lhmac64 },
		{ "dhexchange", ldhexchange },
		{ "dhsecret", ldhsecret },
		{ "base64encode", lb64encode },
		{ "base64decode", lb64decode },
		{ "sha1", lsha1 },
		{ "hmac_sha1", lhmac_sha1 },
		{ "hmac_hash", lhmac_hash },
		{ "aesencode", laesencode },
		{ "aesdecode", laesdecode },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
