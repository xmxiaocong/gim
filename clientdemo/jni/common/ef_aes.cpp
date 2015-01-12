#include "ef_aes.h"
#include "ef_md5.h"
#include "ef_hex.h"
#include "rijndael-api-fst.h"

namespace ef{
	int32 aes_encrypt(const std::string& src, 
		const std::string& key, std::string& dst){
			int ret = 0;
			keyInstance keyist;
			u8 keymd5[16] = {0};
			u8 keymat[33] = {0};
			ef::MD5(keymd5, (const u8*)key.data(), key.size());
			ef::bytes_to_hexs((const char*)keymd5, 16, (char*)keymat, 32);
			ret = makeKey(&keyist, DIR_ENCRYPT, 128, keymat);
			if(ret < 0){
				return ret;
			}

			cipherInstance cnst;
			ret = cipherInit(&cnst, MODE_ECB, (char*)keymat);
			if(ret < 0){
				return ret;
			}
			int len = src.size();
			dst.resize(len + 16 - len % 16);
			BYTE* encout = (BYTE*)dst.data();
			ret = padEncrypt(&cnst, &keyist, (BYTE*)src.data(), len, encout);
			if(ret < 0)
				return ret;
			dst.resize(ret);
			return ret;
	}

	int32 aes_decrypt(const std::string& src, 
		const std::string& key, std::string& dst){
			int ret = 0;
			keyInstance keyist;
			u8 keymd5[16];
			u8 keymat[33] = {0};
			ef::MD5(keymd5, (const u8*)key.data(), key.size());
			ef::bytes_to_hexs((const char*)keymd5, 16, (char*)keymat, 32);
			ret = makeKey(&keyist, DIR_DECRYPT, 128, keymat);
			if(ret < 0){
				return ret;
			}

			cipherInstance cnst;
			ret = cipherInit(&cnst, MODE_ECB, (char*)keymat);
			if(ret < 0){
				return ret;
			}
			int len = src.size();
			dst.resize(len + 16 - len % 16);
			BYTE* encout = (BYTE*)dst.data();
			ret = padDecrypt(&cnst, &keyist, (BYTE*)src.data(), len, encout);
			if(ret < 0)
				return ret;
			dst.resize(ret);
			return ret;
	}
};