#include "ef_loop_buf.h"
#include <cassert>
#include <string.h>

namespace	ef{

		loop_buf::loop_buf(int32 cap):
			m_cap(cap),
			m_size(0),
			m_start(0)
		{
			assert(cap);
			m_buf = (uint8*)new uint8[cap];
		}

		loop_buf::~loop_buf(){
			delete [] m_buf;
		}

		int32	loop_buf::resize(int32 cap){
			assert(cap);
			int32	ret = 0;
			uint8	*buf = (uint8*)new uint8[cap];	
			ret = read(buf, cap);
			uint8	*oldbuf = m_buf;
			m_buf = buf;
			delete	[] oldbuf;			
			m_size = ret;
			m_start = 0;
			m_cap = cap;
			return	ret;
		}

		int32	loop_buf::read(uint8 *buf, int32 len){
			int32	realread = len;
			if(m_size <= len)
				realread = m_size;
			int32	firsthalf = m_cap - m_start;			
			if(firsthalf < realread){
				if(buf){
					memmove(buf, m_buf + m_start, firsthalf);
					memmove(buf + firsthalf, m_buf, realread - firsthalf);
				}
				m_start = realread - firsthalf;
			}else{
				if(buf){
					memmove(buf, m_buf + m_start, realread);
				}
				m_start += realread;
			}

			m_size -= realread;					
			if(m_size <= 0)
				m_start = 0;

			return	realread;
		}

		int32	loop_buf::first_half_len() const{
			int32	firsthalf = m_cap - m_start;				
			if(firsthalf < m_size){
				return firsthalf;	
			}
			return	m_size;
		}

		const uint8* loop_buf::first_half() const{
			return	m_buf + m_start;
		}

		int32	loop_buf::write(const uint8 *buf, int32 len){
			int32	realwrite = len;
			if(realwrite > m_cap - m_size)
				realwrite = m_cap - m_size;
			int32	stop = (m_start + m_size) % m_cap;
			int32	firsthalf = m_cap - stop;
			if(firsthalf < realwrite){
				if(buf){
					memmove(m_buf + stop, buf, firsthalf);
					memmove(m_buf, buf + firsthalf, realwrite - firsthalf);
				}
			}else{
				if(buf){
					memmove(m_buf + stop, buf, realwrite);
				}
			}	
			m_size += realwrite;
			return	realwrite;
		}

		int32	loop_buf::next_part_len() const{

			int32	leftsize = m_cap - m_size;
			int32	stop = (m_start + m_size) % m_cap;
			int32	nextpart = m_cap - stop;
			if(nextpart < leftsize){
				return	nextpart;
			}
			return	leftsize;
		}

		uint8*	loop_buf::next_part(){
			int32	stop = (m_start + m_size) % m_cap;
			return	m_buf + stop;	
		}
		
		int32	loop_buf::auto_resize_write(const uint8 *buf, int32 len){
			if(m_size + len > m_cap){
				int32 nlen = m_cap > len ? m_cap * 2 : len * 2;
				resize(nlen);
			}
			return	write(buf, len);
		}

		int32	loop_buf::peek(uint8 *buf, int32 len) const{
			int32	realread = len;
			if(m_size <= len)
				realread = m_size;
			int32	firsthalf = m_cap - m_start;			
			if(firsthalf < realread){
				memmove(buf, m_buf + m_start, firsthalf);
				memmove(buf + firsthalf, m_buf, realread - firsthalf);	
			}else{
				memmove(buf, m_buf + m_start, realread);
			}
		
			return	realread;

		}

		int32	loop_buf::clear(){
			int32	ret = m_size;
			m_size = 0;
			m_start = 0;

			return	ret;
		}

		int32	loop_buf::capacity() const{
			return	m_cap;
		}

};

