#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "d3des.h"


static void scrunch(unsigned char*, unsigned long*);
static void unscrun(unsigned long*, unsigned char*);
static void desfunc(unsigned long*, unsigned long*);
static void cookey(unsigned long*);


static unsigned long KnL[32] = { 0L };
static unsigned long KnR[32] = { 0L };
static unsigned long Kn3[32] = { 0L };
static unsigned char Df_Key[24] = {
	0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
	0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
	0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67 };

static unsigned short bytebit[8] = {
	0200, 0100, 040, 020, 010, 04, 02, 01 };

static unsigned long bigbyte[24] = {
	0x800000L,	0x400000L,	0x200000L, 	0x100000L,
	0x80000L,	0x40000L,	0x20000L,	0x10000L,
	0x8000L,	0x4000L,	0x2000L,	0x1000L,
	0x800L,		0x400L,		0x200L,		0x100L,
	0x80L,		0x40L,		0x20L,		0x10L,
	0x8L,		0x4L,		0x2L,		0x1L };

/* Use the key schedule specified in the Standard (ANSI X3.92-1981). */

static unsigned char pc1[56] = {
	56, 48, 40, 32, 24, 16,  8,	 0, 57, 49, 41, 33, 25, 17,
	 9,  1, 58, 50, 42, 34, 26,	18, 10,  2, 59, 51, 43, 35,
	62, 54, 46, 38, 30, 22, 14,	 6, 61, 53, 45, 37, 29, 21,
	13,  5, 60, 52, 44, 36, 28,	20, 12,  4, 27, 19, 11,  3 };

static unsigned char totrot[16] = {
	1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

static unsigned char pc2[48] = {
	13, 16, 10, 23,  0,  4,	 2, 27, 14,  5, 20,  9,
	22, 18, 11,  3, 25,  7,	15,  6, 26, 19, 12,  1,
	40, 51, 30, 36, 46, 54,	29, 39, 50, 44, 32, 47,
	43, 48, 38, 55, 33, 52,	45, 41, 49, 35, 28, 31 };

void deskey(key, edf)	/* Thanks to James Gillogly & Phil Karn! */
unsigned char* key;
short edf;
{
	register int i, j, l, m, n;
	unsigned char pc1m[56], pcr[56];
	unsigned long kn[32];

	for (j = 0; j < 56; j++) {
		l = pc1[j];
		m = l & 07;
		pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1 : 0;
	}
	for (i = 0; i < 16; i++) {
		if (edf == DE1) m = (15 - i) << 1;
		else m = i << 1;
		n = m + 1;
		kn[m] = kn[n] = 0L;
		for (j = 0; j < 28; j++) {
			l = j + totrot[i];
			if (l < 28) pcr[j] = pc1m[l];
			else pcr[j] = pc1m[l - 28];
		}
		for (j = 28; j < 56; j++) {
			l = j + totrot[i];
			if (l < 56) pcr[j] = pc1m[l];
			else pcr[j] = pc1m[l - 28];
		}
		for (j = 0; j < 24; j++) {
			if (pcr[pc2[j]]) kn[m] |= bigbyte[j];
			if (pcr[pc2[j + 24]]) kn[n] |= bigbyte[j];
		}
	}
	cookey(kn);
	return;
}

static void cookey(raw1)
register unsigned long* raw1;
{
	register unsigned long* cook, * raw0;
	unsigned long dough[32];
	register int i;

	cook = dough;
	for (i = 0; i < 16; i++, raw1++) {
		raw0 = raw1++;
		*cook = (*raw0 & 0x00fc0000L) << 6;
		*cook |= (*raw0 & 0x00000fc0L) << 10;
		*cook |= (*raw1 & 0x00fc0000L) >> 10;
		*cook++ |= (*raw1 & 0x00000fc0L) >> 6;
		*cook = (*raw0 & 0x0003f000L) << 12;
		*cook |= (*raw0 & 0x0000003fL) << 16;
		*cook |= (*raw1 & 0x0003f000L) >> 4;
		*cook++ |= (*raw1 & 0x0000003fL);
	}
	usekey(dough);
	return;
}

void cpkey(into)
register unsigned long* into;
{
	register unsigned long* from, * endp;

	from = KnL, endp = &KnL[32];
	while (from < endp) *into++ = *from++;
	return;
}

void usekey(from)
register unsigned long* from;
{
	register unsigned long* to, * endp;

	to = KnL, endp = &KnL[32];
	while (to < endp) *to++ = *from++;
	return;
}



/*des*/
void des(inblock, outblock)
unsigned char* inblock, * outblock;
{
	unsigned long work[2];

	scrunch(inblock, work);
	desfunc(work, KnL);
	unscrun(work, outblock);
	return;
}

/**/
static void scrunch(outof, into)
register unsigned char* outof;
register unsigned long* into;
{
	*into = (*outof++ & 0xffL) << 24;
	*into |= (*outof++ & 0xffL) << 16;
	*into |= (*outof++ & 0xffL) << 8;
	*into++ |= (*outof++ & 0xffL);
	*into = (*outof++ & 0xffL) << 24;
	*into |= (*outof++ & 0xffL) << 16;
	*into |= (*outof++ & 0xffL) << 8;
	*into |= (*outof & 0xffL);
	return;
}
/**/
static void unscrun(outof, into)
register unsigned long* outof;
register unsigned char* into;
{
	*into++ = (*outof >> 24) & 0xffL;
	*into++ = (*outof >> 16) & 0xffL;
	*into++ = (*outof >> 8) & 0xffL;
	*into++ = *outof++ & 0xffL;
	*into++ = (*outof >> 24) & 0xffL;
	*into++ = (*outof >> 16) & 0xffL;
	*into++ = (*outof >> 8) & 0xffL;
	*into = *outof & 0xffL;
	return;
}

static unsigned long SP1[64] = {
	0x01010400L, 0x00000000L, 0x00010000L, 0x01010404L,
	0x01010004L, 0x00010404L, 0x00000004L, 0x00010000L,
	0x00000400L, 0x01010400L, 0x01010404L, 0x00000400L,
	0x01000404L, 0x01010004L, 0x01000000L, 0x00000004L,
	0x00000404L, 0x01000400L, 0x01000400L, 0x00010400L,
	0x00010400L, 0x01010000L, 0x01010000L, 0x01000404L,
	0x00010004L, 0x01000004L, 0x01000004L, 0x00010004L,
	0x00000000L, 0x00000404L, 0x00010404L, 0x01000000L,
	0x00010000L, 0x01010404L, 0x00000004L, 0x01010000L,
	0x01010400L, 0x01000000L, 0x01000000L, 0x00000400L,
	0x01010004L, 0x00010000L, 0x00010400L, 0x01000004L,
	0x00000400L, 0x00000004L, 0x01000404L, 0x00010404L,
	0x01010404L, 0x00010004L, 0x01010000L, 0x01000404L,
	0x01000004L, 0x00000404L, 0x00010404L, 0x01010400L,
	0x00000404L, 0x01000400L, 0x01000400L, 0x00000000L,
	0x00010004L, 0x00010400L, 0x00000000L, 0x01010004L };

static unsigned long SP2[64] = {
	0x80108020L, 0x80008000L, 0x00008000L, 0x00108020L,
	0x00100000L, 0x00000020L, 0x80100020L, 0x80008020L,
	0x80000020L, 0x80108020L, 0x80108000L, 0x80000000L,
	0x80008000L, 0x00100000L, 0x00000020L, 0x80100020L,
	0x00108000L, 0x00100020L, 0x80008020L, 0x00000000L,
	0x80000000L, 0x00008000L, 0x00108020L, 0x80100000L,
	0x00100020L, 0x80000020L, 0x00000000L, 0x00108000L,
	0x00008020L, 0x80108000L, 0x80100000L, 0x00008020L,
	0x00000000L, 0x00108020L, 0x80100020L, 0x00100000L,
	0x80008020L, 0x80100000L, 0x80108000L, 0x00008000L,
	0x80100000L, 0x80008000L, 0x00000020L, 0x80108020L,
	0x00108020L, 0x00000020L, 0x00008000L, 0x80000000L,
	0x00008020L, 0x80108000L, 0x00100000L, 0x80000020L,
	0x00100020L, 0x80008020L, 0x80000020L, 0x00100020L,
	0x00108000L, 0x00000000L, 0x80008000L, 0x00008020L,
	0x80000000L, 0x80100020L, 0x80108020L, 0x00108000L };

static unsigned long SP3[64] = {
	0x00000208L, 0x08020200L, 0x00000000L, 0x08020008L,
	0x08000200L, 0x00000000L, 0x00020208L, 0x08000200L,
	0x00020008L, 0x08000008L, 0x08000008L, 0x00020000L,
	0x08020208L, 0x00020008L, 0x08020000L, 0x00000208L,
	0x08000000L, 0x00000008L, 0x08020200L, 0x00000200L,
	0x00020200L, 0x08020000L, 0x08020008L, 0x00020208L,
	0x08000208L, 0x00020200L, 0x00020000L, 0x08000208L,
	0x00000008L, 0x08020208L, 0x00000200L, 0x08000000L,
	0x08020200L, 0x08000000L, 0x00020008L, 0x00000208L,
	0x00020000L, 0x08020200L, 0x08000200L, 0x00000000L,
	0x00000200L, 0x00020008L, 0x08020208L, 0x08000200L,
	0x08000008L, 0x00000200L, 0x00000000L, 0x08020008L,
	0x08000208L, 0x00020000L, 0x08000000L, 0x08020208L,
	0x00000008L, 0x00020208L, 0x00020200L, 0x08000008L,
	0x08020000L, 0x08000208L, 0x00000208L, 0x08020000L,
	0x00020208L, 0x00000008L, 0x08020008L, 0x00020200L };

static unsigned long SP4[64] = {
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
	0x00802080L, 0x00800081L, 0x00800001L, 0x00002001L,
	0x00000000L, 0x00802000L, 0x00802000L, 0x00802081L,
	0x00000081L, 0x00000000L, 0x00800080L, 0x00800001L,
	0x00000001L, 0x00002000L, 0x00800000L, 0x00802001L,
	0x00000080L, 0x00800000L, 0x00002001L, 0x00002080L,
	0x00800081L, 0x00000001L, 0x00002080L, 0x00800080L,
	0x00002000L, 0x00802080L, 0x00802081L, 0x00000081L,
	0x00800080L, 0x00800001L, 0x00802000L, 0x00802081L,
	0x00000081L, 0x00000000L, 0x00000000L, 0x00802000L,
	0x00002080L, 0x00800080L, 0x00800081L, 0x00000001L,
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
	0x00802081L, 0x00000081L, 0x00000001L, 0x00002000L,
	0x00800001L, 0x00002001L, 0x00802080L, 0x00800081L,
	0x00002001L, 0x00002080L, 0x00800000L, 0x00802001L,
	0x00000080L, 0x00800000L, 0x00002000L, 0x00802080L };

static unsigned long SP5[64] = {
	0x00000100L, 0x02080100L, 0x02080000L, 0x42000100L,
	0x00080000L, 0x00000100L, 0x40000000L, 0x02080000L,
	0x40080100L, 0x00080000L, 0x02000100L, 0x40080100L,
	0x42000100L, 0x42080000L, 0x00080100L, 0x40000000L,
	0x02000000L, 0x40080000L, 0x40080000L, 0x00000000L,
	0x40000100L, 0x42080100L, 0x42080100L, 0x02000100L,
	0x42080000L, 0x40000100L, 0x00000000L, 0x42000000L,
	0x02080100L, 0x02000000L, 0x42000000L, 0x00080100L,
	0x00080000L, 0x42000100L, 0x00000100L, 0x02000000L,
	0x40000000L, 0x02080000L, 0x42000100L, 0x40080100L,
	0x02000100L, 0x40000000L, 0x42080000L, 0x02080100L,
	0x40080100L, 0x00000100L, 0x02000000L, 0x42080000L,
	0x42080100L, 0x00080100L, 0x42000000L, 0x42080100L,
	0x02080000L, 0x00000000L, 0x40080000L, 0x42000000L,
	0x00080100L, 0x02000100L, 0x40000100L, 0x00080000L,
	0x00000000L, 0x40080000L, 0x02080100L, 0x40000100L };

static unsigned long SP6[64] = {
	0x20000010L, 0x20400000L, 0x00004000L, 0x20404010L,
	0x20400000L, 0x00000010L, 0x20404010L, 0x00400000L,
	0x20004000L, 0x00404010L, 0x00400000L, 0x20000010L,
	0x00400010L, 0x20004000L, 0x20000000L, 0x00004010L,
	0x00000000L, 0x00400010L, 0x20004010L, 0x00004000L,
	0x00404000L, 0x20004010L, 0x00000010L, 0x20400010L,
	0x20400010L, 0x00000000L, 0x00404010L, 0x20404000L,
	0x00004010L, 0x00404000L, 0x20404000L, 0x20000000L,
	0x20004000L, 0x00000010L, 0x20400010L, 0x00404000L,
	0x20404010L, 0x00400000L, 0x00004010L, 0x20000010L,
	0x00400000L, 0x20004000L, 0x20000000L, 0x00004010L,
	0x20000010L, 0x20404010L, 0x00404000L, 0x20400000L,
	0x00404010L, 0x20404000L, 0x00000000L, 0x20400010L,
	0x00000010L, 0x00004000L, 0x20400000L, 0x00404010L,
	0x00004000L, 0x00400010L, 0x20004010L, 0x00000000L,
	0x20404000L, 0x20000000L, 0x00400010L, 0x20004010L };

static unsigned long SP7[64] = {
	0x00200000L, 0x04200002L, 0x04000802L, 0x00000000L,
	0x00000800L, 0x04000802L, 0x00200802L, 0x04200800L,
	0x04200802L, 0x00200000L, 0x00000000L, 0x04000002L,
	0x00000002L, 0x04000000L, 0x04200002L, 0x00000802L,
	0x04000800L, 0x00200802L, 0x00200002L, 0x04000800L,
	0x04000002L, 0x04200000L, 0x04200800L, 0x00200002L,
	0x04200000L, 0x00000800L, 0x00000802L, 0x04200802L,
	0x00200800L, 0x00000002L, 0x04000000L, 0x00200800L,
	0x04000000L, 0x00200800L, 0x00200000L, 0x04000802L,
	0x04000802L, 0x04200002L, 0x04200002L, 0x00000002L,
	0x00200002L, 0x04000000L, 0x04000800L, 0x00200000L,
	0x04200800L, 0x00000802L, 0x00200802L, 0x04200800L,
	0x00000802L, 0x04000002L, 0x04200802L, 0x04200000L,
	0x00200800L, 0x00000000L, 0x00000002L, 0x04200802L,
	0x00000000L, 0x00200802L, 0x04200000L, 0x00000800L,
	0x04000002L, 0x04000800L, 0x00000800L, 0x00200002L };

static unsigned long SP8[64] = {
	0x10001040L, 0x00001000L, 0x00040000L, 0x10041040L,
	0x10000000L, 0x10001040L, 0x00000040L, 0x10000000L,
	0x00040040L, 0x10040000L, 0x10041040L, 0x00041000L,
	0x10041000L, 0x00041040L, 0x00001000L, 0x00000040L,
	0x10040000L, 0x10000040L, 0x10001000L, 0x00001040L,
	0x00041000L, 0x00040040L, 0x10040040L, 0x10041000L,
	0x00001040L, 0x00000000L, 0x00000000L, 0x10040040L,
	0x10000040L, 0x10001000L, 0x00041040L, 0x00040000L,
	0x00041040L, 0x00040000L, 0x10041000L, 0x00001000L,
	0x00000040L, 0x10040040L, 0x00001000L, 0x00041040L,
	0x10001000L, 0x00000040L, 0x10000040L, 0x10040000L,
	0x10040040L, 0x10000000L, 0x00040000L, 0x10001040L,
	0x00000000L, 0x10041040L, 0x00040040L, 0x10000040L,
	0x10040000L, 0x10001000L, 0x10001040L, 0x00000000L,
	0x10041040L, 0x00041000L, 0x00041000L, 0x00001040L,
	0x00001040L, 0x00040040L, 0x10000000L, 0x10041000L };

static void desfunc(block, keys)
register unsigned long* block, * keys;
{
	register unsigned long fval, work, right, leftt;
	register int round;

	leftt = block[0];
	right = block[1];
	work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL;
	right ^= work;
	leftt ^= (work << 4);
	work = ((leftt >> 16) ^ right) & 0x0000ffffL;
	right ^= work;
	leftt ^= (work << 16);
	work = ((right >> 2) ^ leftt) & 0x33333333L;
	leftt ^= work;
	right ^= (work << 2);
	work = ((right >> 8) ^ leftt) & 0x00ff00ffL;
	leftt ^= work;
	right ^= (work << 8);
	right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL;
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL;

	for (round = 0; round < 8; round++) {
		work = (right << 28) | (right >> 4);
		work ^= *keys++;
		fval = SP7[work & 0x3fL];
		fval |= SP5[(work >> 8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work = right ^ *keys++;
		fval |= SP8[work & 0x3fL];
		fval |= SP6[(work >> 8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		leftt ^= fval;
		work = (leftt << 28) | (leftt >> 4);
		work ^= *keys++;
		fval = SP7[work & 0x3fL];
		fval |= SP5[(work >> 8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work = leftt ^ *keys++;
		fval |= SP8[work & 0x3fL];
		fval |= SP6[(work >> 8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		right ^= fval;
	}

	right = (right << 31) | (right >> 1);
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = (leftt << 31) | (leftt >> 1);
	work = ((leftt >> 8) ^ right) & 0x00ff00ffL;
	right ^= work;
	leftt ^= (work << 8);
	work = ((leftt >> 2) ^ right) & 0x33333333L;
	right ^= work;
	leftt ^= (work << 2);
	work = ((right >> 16) ^ leftt) & 0x0000ffffL;
	leftt ^= work;
	right ^= (work << 16);
	work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL;
	leftt ^= work;
	right ^= (work << 4);
	*block++ = right;
	*block = leftt;
	return;
}

#ifdef D2_DES

void des2key(hexkey, mode)		/* stomps on Kn3 too */
unsigned char* hexkey;			/* unsigned char[16] */
short mode;
{
	short revmod;

	revmod = (mode == EN0) ? DE1 : EN0;
	deskey(&hexkey[8], revmod);
	cpkey(KnR);
	deskey(hexkey, mode);
	cpkey(Kn3);					/* Kn3 = KnL */
	return;
}

void Ddes(from, into)
unsigned char* from, * into;		/* unsigned char[8] */
{
	unsigned long work[2];

	scrunch(from, work);
	desfunc(work, KnL);
	desfunc(work, KnR);
	desfunc(work, Kn3);
	unscrun(work, into);
	return;
}

void D2des(from, into)
unsigned char* from;			/* unsigned char[16] */
unsigned char* into;			/* unsigned char[16] */
{
	unsigned long* right, * l1, swap;
	unsigned long leftt[2], bufR[2];

	right = bufR;
	l1 = &leftt[1];
	scrunch(from, leftt);
	scrunch(&from[8], right);
	desfunc(leftt, KnL);
	desfunc(right, KnL);
	swap = *l1;
	*l1 = *right;
	*right = swap;
	desfunc(leftt, KnR);
	desfunc(right, KnR);
	swap = *l1;
	*l1 = *right;
	*right = swap;
	desfunc(leftt, Kn3);
	desfunc(right, Kn3);
	unscrun(leftt, into);
	unscrun(right, &into[8]);
	return;
}

void makekey(aptr, kptr)
register char* aptr;				/* NULL-terminated  */
register unsigned char* kptr;		/* unsigned char[8] */
{
	register unsigned char* store;
	register int first, i;
	unsigned long savek[96];

	cpDkey(savek);
	des2key(Df_Key, EN0);
	for (i = 0; i < 8; i++) kptr[i] = Df_Key[i];
	first = 1;
	while ((*aptr != '\0') || first) {
		store = kptr;
		for (i = 0; i < 8 && (*aptr != '\0'); i++) {
			*store++ ^= *aptr & 0x7f;
			*aptr++ = '\0';
		}
		Ddes(kptr, kptr);
		first = 0;
	}
	useDkey(savek);
	return;
}

void make2key(aptr, kptr)
register char* aptr;				/* NULL-terminated   */
register unsigned char* kptr;		/* unsigned char[16] */
{
	register unsigned char* store;
	register int first, i;
	unsigned long savek[96];

	cpDkey(savek);
	des2key(Df_Key, EN0);
	for (i = 0; i < 16; i++) kptr[i] = Df_Key[i];
	first = 1;
	while ((*aptr != '\0') || first) {
		store = kptr;
		for (i = 0; i < 16 && (*aptr != '\0'); i++) {
			*store++ ^= *aptr & 0x7f;
			*aptr++ = '\0';
		}
		D2des(kptr, kptr);
		first = 0;
	}
	useDkey(savek);
	return;
}

#ifndef D3_DES	/* D2_DES only */
#ifdef  D2_DES	/* iff D2_DES! */

void cp2key(into)
register unsigned long* into;	/* unsigned long[64] */
{
	register unsigned long* from, * endp;

	cpkey(into);
	into = &into[32];
	from = KnR, endp = &KnR[32];
	while (from < endp) *into++ = *from++;
	return;
}

void use2key(from)				/* stomps on Kn3 too */
register unsigned long* from;	/* unsigned long[64] */
{
	register unsigned long* to, * endp;

	usekey(from);
	from = &from[32];
	to = KnR, endp = &KnR[32];
	while (to < endp) *to++ = *from++;
	cpkey(Kn3);					/* Kn3 = KnL */
	return;
}

#endif	/* iff D2_DES */
#else	/* D3_DES too */

static void D3des(unsigned char*, unsigned char*);

void des3key(hexkey, mode)
unsigned char* hexkey;			/* unsigned char[24] */
short mode;
{
	unsigned char* first, * third;
	short revmod;

	if (mode == EN0) {
		revmod = DE1;
		first = hexkey;
		third = &hexkey[16];
	}
	else {
		revmod = EN0;
		first = &hexkey[16];
		third = hexkey;
	}
	deskey(&hexkey[8], revmod);
	cpkey(KnR);
	deskey(third, mode);
	cpkey(Kn3);
	deskey(first, mode);
	return;
}

void cp3key(into)
register unsigned long* into;	/* unsigned long[96] */
{
	register unsigned long* from, * endp;

	cpkey(into);
	into = &into[32];
	from = KnR, endp = &KnR[32];
	while (from < endp) *into++ = *from++;
	from = Kn3, endp = &Kn3[32];
	while (from < endp) *into++ = *from++;
	return;
}

void use3key(from)
register unsigned long* from;	/* unsigned long[96] */
{
	register unsigned long* to, * endp;

	usekey(from);
	from = &from[32];
	to = KnR, endp = &KnR[32];
	while (to < endp) *to++ = *from++;
	to = Kn3, endp = &Kn3[32];
	while (to < endp) *to++ = *from++;
	return;
}

static void D3des(from, into)	/* amateur theatrics */
unsigned char* from;			/* unsigned char[24] */
unsigned char* into;			/* unsigned char[24] */
{
	unsigned long swap, leftt[2], middl[2], right[2];

	scrunch(from, leftt);
	scrunch(&from[8], middl);
	scrunch(&from[16], right);
	desfunc(leftt, KnL);
	desfunc(middl, KnL);
	desfunc(right, KnL);
	swap = leftt[1];
	leftt[1] = middl[0];
	middl[0] = swap;
	swap = middl[1];
	middl[1] = right[0];
	right[0] = swap;
	desfunc(leftt, KnR);
	desfunc(middl, KnR);
	desfunc(right, KnR);
	swap = leftt[1];
	leftt[1] = middl[0];
	middl[0] = swap;
	swap = middl[1];
	middl[1] = right[0];
	right[0] = swap;
	desfunc(leftt, Kn3);
	desfunc(middl, Kn3);
	desfunc(right, Kn3);
	unscrun(leftt, into);
	unscrun(middl, &into[8]);
	unscrun(right, &into[16]);
	return;
}

void make3key(aptr, kptr)
register char* aptr;				/* NULL-terminated   */
register unsigned char* kptr;		/* unsigned char[24] */
{
	register unsigned char* store;
	register int first, i;
	unsigned long savek[96];

	cp3key(savek);
	des3key(Df_Key, EN0);
	for (i = 0; i < 24; i++) kptr[i] = Df_Key[i];
	first = 1;
	while ((*aptr != '\0') || first) {
		store = kptr;
		for (i = 0; i < 24 && (*aptr != '\0'); i++) {
			*store++ ^= *aptr & 0x7f;
			*aptr++ = '\0';
		}
		D3des(kptr, kptr);
		first = 0;
	}
	use3key(savek);
	return;
}

#endif	/* D3_DES */
#endif	/* D2_DES */

int __fastcall CharToInt(int a1)
{
	int v1; // r1
	int v2; // r2
	int result; // r0

	v1 = a1;
	v2 = (unsigned __int8)a1;
	if ((unsigned int)((a1 << 24) - 805306368) >> 24 <= 9)
		return a1 - 48;
	result = 0;
	if (((v2 - 65) & 0xFFu) <= 5)
		result = v1 - 55;
	return result;
}

/*
 * 使用DES算法加密数据（支持动态长度输出）
 *
 * @param input_data 要加密的输入数据
 * @param data_len 加密数据实际长度（字节数）
 * @param key 加密密钥（NULL则使用默认密钥"abcdef"）
 * @param output [输出参数] 接收加密数据指针的地址
 * @param output_len [输出参数] 接收加密数据长度
 * @return 成功返回1，失败返回0（需调用方释放*output内存）
 */
int _stdcall gfEncryptDynamic(const unsigned char* input_data, unsigned int data_len, const unsigned char* key,
	unsigned char** output, unsigned int* output_len)
{
	const char* actual_key;
	unsigned int encrypted_len = 0;
	char processed_key[128] = { 0 };
	char key_buf[128] = { 0 };
	char* input_buf = NULL;
	char* encrypted_buf = NULL;
	unsigned int block_count, i;

	// 参数检查
	if (!input_data || !output || !output_len) {
		return 0;
	}

	actual_key = key ? key : "abcdef";  // 默认密钥

	// 计算需要的缓冲区大小（8字节对齐）
	block_count = (data_len + 7) / 8;  // 向上取整
	encrypted_len = block_count * 8;    // DES每次加密8字节

	// 分配输入/输出缓冲区
	input_buf = (char*)calloc(encrypted_len, 1);  // +1确保字符串终止
	encrypted_buf = (char*)calloc(encrypted_len, 1);
	if (!input_buf || !encrypted_buf) {
		free(input_buf);
		free(encrypted_buf);
		return 0;
	}

	// 准备数据
	memcpy(input_buf, input_data, data_len);
	strcpy(key_buf, actual_key);

	// 准备加密密钥
	makekey(key_buf, (unsigned char*)processed_key);
	deskey((unsigned char*)processed_key, 0);  // 0表示加密模式

	// 分块加密
	for (i = 0; i < block_count; i++) {
		des((unsigned __int8*)(input_buf + i * 8),
			(unsigned __int8*)(encrypted_buf + i * 8));
	}

	// 设置输出参数
	*output = (unsigned char*)encrypted_buf;
	*output_len = encrypted_len;

	// 清理
	free(input_buf);
	return 1;
}


/*
 * 改进版DES解密函数（与加密函数对称设计）
 * @param input_data 加密数据（二进制格式）
 * @param data_len 加密数据实际长度（字节数）
 * @param key 解密密钥（NULL则使用默认"abcdef"）
 * @param output [输出参数] 接收解密数据指针的地址
 * @param output_len [输出参数] 接收解密数据长度
 * @return 成功返回1，失败返回0（需调用方释放*output内存）
 */
int _stdcall gfDecryptDynamic(const unsigned char* input_data, unsigned int data_len, const unsigned char* key,
	unsigned char** output, unsigned int* output_len)
{
	const char* actual_key;
	unsigned int block_count;
	unsigned int i;
	unsigned char* processed_data = NULL;
	char* decrypted_buf = NULL;
	char key_buf[128] = { 0 };
	char processed_key[128] = { 0 };

	// 参数校验
	if (!input_data || !output || !output_len) {
		return 0;
	}

	// 设置实际使用的密钥
	actual_key = key ? key : "abcdef";

	// 计算数据长度和块数（DES每次处理8字节）
	block_count = (data_len + 7) / 8;

	// 分配缓冲区
	processed_data = (unsigned char*)calloc(block_count * 8, 1);
	decrypted_buf = (char*)calloc(block_count * 8 + 1, 1); // +1用于字符串终止符
	if (!processed_data || !decrypted_buf) {
		free(processed_data);
		free(decrypted_buf);
		return 0;
	}

	// 拷贝加密数据（自动填充0到8字节对齐）
	memcpy(processed_data, input_data, data_len);

	// 准备密钥（使用独立缓冲区）
	strncpy(key_buf, actual_key, sizeof(key_buf) - 1);
	makekey(key_buf, (unsigned char*)processed_key); // 使用不同变量存储处理后的密钥
	deskey((unsigned char*)processed_key, 1); // 1表示解密模式

	// 分块解密
	for (i = 0; i < block_count; i++) {
		des(processed_data + i * 8, (unsigned char*)(decrypted_buf + i * 8));
	}

	// 设置输出参数
	*output = decrypted_buf;
	*output_len = block_count * 8;

	// 清理临时缓冲区
	free(processed_data);

	return 1;
}