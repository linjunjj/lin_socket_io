#ifndef __UUID_H__
#define __UUID_H__

#include <stdio.h>
#include <stdint.h>
#include <utils/utility.h>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

#include <string>

namespace lin_io
{
using namespace std;
    //uuid version4，用于随机的traceid之类，不能用于唯一码生成等场景
    //https://github.com/rxi/uuid4
    class Uuid
    {
    public:
        static Uuid& Instance()
        {
            static Uuid ins;
            return ins;
        }
        Uuid()
        {
            UUID4Init();
        }

        inline string Create()
        {
            static const char *strTemplate = "xxxxxxxxxx4xxxyxxxxxxxxx";
            static const char *chars = "0123456789abcdef";

            string ret;
            union { unsigned char b[16]; uint64_t word[2]; } s;
            const char *p;
            int i, n;
            /* get random */
            s.word[0] = xorshift128plus(seed);
            s.word[1] = xorshift128plus(seed);
            /* build string */
            p = strTemplate;
            i = 0;
            while (*p)
            {
                n = s.b[i >> 1];
                n = (i & 1) ? (n >> 4) : (n & 0xf);
                switch (*p)
                {
                case 'x': ret += chars[n];              i++;  break;   //x随机
                case 'y': ret += chars[(n & 0x3) + 8];  i++;  break;   //y只能为8、9、a、b
                default: ret += *p;                                    //版本4不能变
                }
                p++;
            }
            return ret;
        }
    private:
        int UUID4Init(void)
        {
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
            int res;
            FILE *fp = fopen("/dev/urandom", "rb");
            if (!fp)
            {
                return -1;
            }
            res = fread(seed, 1, sizeof(seed), fp);
            fclose(fp);
            if (res != sizeof(seed))
            {
                return -1;
            }

#elif defined(_WIN32)
            int res;
            HCRYPTPROV hCryptProv;
            res = CryptAcquireContext(
                &hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
            if (!res)
            {
                return -1;
            }
            res = CryptGenRandom(hCryptProv, (DWORD) sizeof(seed), (PBYTE)seed);
            CryptReleaseContext(hCryptProv, 0);
            if (!res)
            {
                return -1;
            }
#else
#error "unsupported platform"
#endif
            return 0;
        }

        static uint64_t xorshift128plus(uint64_t *s)
        {
            /* http://xorshift.di.unimi.it/xorshift128plus.c */
            uint64_t s1 = s[0];
            const uint64_t s0 = s[1];
            s[0] = s0;
            s1 ^= s1 << 23;
            s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
            return s[1] + s0;
        }

        uint64_t seed[2];
    };
}

#endif
