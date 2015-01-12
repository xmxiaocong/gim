#ifndef	EF_LOOP_BUF_H
#define	EF_LOOP_BUF_H

#include"ef_btype.h"

namespace	ef{

class	loop_buf{
	public:
		loop_buf(int32	cap = 1024);
		~loop_buf();

		int32	capacity() const;
		int32	resize(int32 cap);
		//if buf is null, just drop data
		int32	read(uint8 *buf, int32 len);
		//if buf is null, just inc size
		int32	write(const uint8 *buf, int32 len);
		int32	auto_resize_write(const uint8 *buf, int32 len);
		int32	peek(uint8 *buf, int32 len) const;
		int32	first_half_len() const;
		const uint8* first_half() const;
		int32	next_part_len() const;
		uint8* next_part();	
		int32	clear();
		int32	size() const{
			return	m_size;
		}

	private:
		uint8	*m_buf;
		int32	m_cap;
		int32	m_size;
		int32	m_start;
		
};

};


#endif/**/
