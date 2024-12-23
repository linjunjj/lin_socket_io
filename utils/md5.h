#ifndef __F25523BFFCD844868F98136566EFCA80_H
#define __F25523BFFCD844868F98136566EFCA80_H
//blog.csdn.net/flydream0/article/details/7045429

#include <string>
#include <fstream>

/* Type define */
typedef unsigned char byte;

/* MD5 declaration. */
class MD5
{
public:
    /* Default construct. */
	MD5()
    {
        reset();
    }
	MD5(const void *input, size_t length)
    {
        reset();
        update(input, length);
    }

	MD5(const std::string &str)
    {
        reset();
        update(str);
    }

	MD5(std::ifstream &in)
    {
        reset();
        update(in);
    }

    const byte* digest()
    {
        if (!m_bFinished)
        {
            m_bFinished = true;
            final();
        }
        return m_digest;
    }

    void reset()
    {
        m_bFinished = false;
        /* reset number of bits. */
        m_count[0] = m_count[1] = 0;
        /* Load magic initialization constants. */
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
    }

    void update(const void *input, size_t length)
    {
        update((const byte*)input, length);
    }

    void update(const std::string &str)
    {
        update((const byte*)str.c_str(), str.length());
    }

    void update(std::ifstream &in)
    {
        if (!in)
        {
            return;
        }
        std::streamsize length;
        char buffer[BUFFER_SIZE];
        while (!in.eof())
        {
            in.read(buffer, BUFFER_SIZE);
            length = in.gcount();
            if (length > 0)
            {
                update(buffer, (size_t)length);
            }
        }
        in.close();
    }

    std::string toString()
    {
        return bytesToHexString(digest(), 16);
    }    
private:
    /* MD5 block update operation. Continues an MD5 message-digest
    operation, processing another message block, and updating the
    context.
    */
    void update(const byte *input, size_t length)
    {

        unsigned int i, index, partLen;

        m_bFinished = false;

        /* Compute number of bytes mod 64 */
        index = (unsigned int)((m_count[0] >> 3) & 0x3f);

        /* update number of bits */
        if ((m_count[0] += ((unsigned int)length << 3)) < ((unsigned int)length << 3))
        {
            m_count[1]++;
        }
        m_count[1] += ((unsigned int)length >> 29);

        partLen = 64 - index;

        /* transform as many times as possible. */
        if (length >= partLen)
        {

            memcpy(&m_buffer[index], input, partLen);
            transform(m_buffer);

            for (i = partLen; i + 63 < length; i += 64)
                transform(&input[i]);
            index = 0;

        }
        else
        {
            i = 0;
        }

        /* Buffer remaining input */
        memcpy(&m_buffer[index], &input[i], length - i);
    }

    /* MD5 finalization. Ends an MD5 message-_digest operation, writing the
    the message _digest and zeroizing the context.
    */
    void final()
    {
        static const byte PADDING[64] = { 0x80 };
        byte bits[8];
        unsigned int oldState[4];
        unsigned int oldCount[2];
        unsigned int index, padLen;

        /* Save current state and count. */
        memcpy(oldState, m_state, 16);
        memcpy(oldCount, m_count, 8);

        /* Save number of bits */
        encode(m_count, bits, 8);

        /* Pad out to 56 mod 64. */
        index = (unsigned int)((m_count[0] >> 3) & 0x3f);
        padLen = (index < 56) ? (56 - index) : (120 - index);
        update(PADDING, padLen);

        /* Append length (before padding) */
        update(bits, 8);

        /* Store state in digest */
        encode(m_state, m_digest, 16);

        /* Restore current state and count. */
        memcpy(m_state, oldState, 16);
        memcpy(m_count, oldCount, 8);
    }

    /* MD5 basic transformation. Transforms _state based on block. */
    void transform(const byte block[64])
    {
        unsigned int a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], x[16];

        decode(block, x, 64);

        /* Round 1 */
        FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
        FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
        FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
        FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
        FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
        FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
        FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
        FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
        FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
        FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
        FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
        FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
        FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
        FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
        FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
        FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

        /* Round 2 */
        GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
        GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
        GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
        GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
        GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
        GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
        GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
        GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
        GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
        GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
        GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
        GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
        GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
        GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
        GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
        GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

        /* Round 3 */
        HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
        HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
        HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
        HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
        HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
        HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
        HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
        HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
        HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
        HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
        HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
        HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
        HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
        HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
        HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
        HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

        /* Round 4 */
        II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
        II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
        II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
        II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
        II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
        II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
        II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
        II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
        II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
        II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
        II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
        II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
        II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
        II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
        II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
        II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
    }

    /* Encodes input (ulong) into output (byte). Assumes length is a multiple of 4.*/
    void encode(const unsigned int *input, byte *output, size_t length)
    {
        for (size_t i = 0, j = 0; j < length; i++, j += 4)
        {
            output[j] = (byte)(input[i] & 0xff);
            output[j + 1] = (byte)((input[i] >> 8) & 0xff);
            output[j + 2] = (byte)((input[i] >> 16) & 0xff);
            output[j + 3] = (byte)((input[i] >> 24) & 0xff);
        }
    }

    /* Decodes input (byte) into output (ulong). Assumes length is a multiple of 4.*/
    void decode(const byte *input, unsigned int *output, size_t length)
    {
        for (size_t i = 0, j = 0; j < length; i++, j += 4)
        {
            output[i] = ((unsigned int)input[j]) | (((unsigned int)input[j + 1]) << 8) |
                (((unsigned int)input[j + 2]) << 16) | (((unsigned int)input[j + 3]) << 24);
        }
    }

    /* Convert byte array to hex string. */
    std::string bytesToHexString(const byte *input, size_t length)
    {
        static const char HEX[16] =
        {
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', 'a', 'b',
            'c', 'd', 'e', 'f'
        };
        std::string str;
        str.reserve(length << 1);
        for (size_t i = 0; i < length; i++)
        {
            int t = input[i];
            int a = t / 16;
            int b = t % 16;
            str.append(1, HEX[a]);
            str.append(1, HEX[b]);
        }
        return str;
    }

    unsigned int F(unsigned int x, unsigned int y, unsigned int z)
    {
        return (((x)& (y)) | ((~x) & (z)));
    }

    unsigned int G(unsigned int x, unsigned int y, unsigned int z)
    {
        return (((x)& (z)) | ((y)& (~z)));
    }

    unsigned int H(unsigned int x, unsigned int y, unsigned int z)
    {
        return ((x) ^ (y) ^ (z));
    }

    unsigned int I(unsigned int x, unsigned int y, unsigned int z)
    {
        return ((y) ^ ((x) | (~z)));
    }
    unsigned int ROTATE_LEFT(unsigned int x, unsigned int n)
    {
        return (((x) << (n)) | ((x) >> (32 - (n))));
    }
    void FF(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, unsigned int s, unsigned int ac)
    {
        a += F(b, c, d) + x+ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    void GG(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, unsigned int s, unsigned int ac)
    {
        a += G(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    void HH(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, unsigned int s, unsigned int ac)
    {
        a += H(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }
    void II(unsigned int &a, unsigned int b, unsigned int c, unsigned int d, unsigned int x, unsigned int s, unsigned int ac)
    {
        a += I(b, c, d) + x + ac;
        a = ROTATE_LEFT(a, s);
        a += b;
    }

private:
	unsigned int m_state[4];	/* state (ABCD) */
	unsigned int m_count[2];	/* number of bits, modulo 2^64 (low-order word first) */
	byte m_buffer[64];	/* input buffer */
	byte m_digest[16];	/* message digest */
    bool m_bFinished;		/* calculate finished ? */
    static const size_t BUFFER_SIZE = 1024;
    static const int S11 = 7;
    static const int S12 = 12;
    static const int S13 = 17;
    static const int S14 = 22;
    static const int S21 = 5;
    static const int S22 = 9;
    static const int S23 = 14;
    static const int S24 = 20;
    static const int S31 = 4;
    static const int S32 = 11;
    static const int S33 = 16;
    static const int S34 = 23;
    static const int S41 = 6;
    static const int S42 = 10;
    static const int S43 = 15;
    static const int S44 = 21;
};

typedef MD5 MD5Utils;

#endif/*__F25523BFFCD844868F98136566EFCA80_H*/
