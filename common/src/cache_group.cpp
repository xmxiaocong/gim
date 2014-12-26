#include "cache_group.h"
#include "base/ef_md5.h"
#include "base/ef_btype.h"
#include "base/ef_utility.h"
#include <stdlib.h>
#include <sstream>

namespace gim {

using namespace ef;

uint64 getIdx(int size, const string &key)
{
	ef::uint8 md5[16];
        ef::MD5_CTX c;

        ef::MD5Init(&c);
        ef::MD5Update(&c, (ef::uint8 *)key.data(), key.size());
        ef::MD5Final(md5, &c);
        //count the md5
        ef::uint64 i = *(ef::uint64*)md5;
        ef::uint64 idx = i % size;
	return idx;
}


};
