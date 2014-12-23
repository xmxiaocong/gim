#include "ef_loop_buf.h"
#include <cassert>
#include <string.h>

namespace	ef{

	int32	frame::peek(uint8 *buf, int32 len) const{
		int32	realread = len;
		if(m_size <= len)
			realread = m_size;
		memmove(buf, m_buf + m_start, realread);

		return realread;

	}

	int32	frame::read(uint8 *buf, int32 len){
		int32	realread = peek(buf, len);
		m_start += realread;
		m_size -= realread;					
		if(m_size <= 0)
			m_start = 0;

		return realread;
	}

	int32	frame::write(const uint8 *buf, int32 len){
		int32	realwrite = len;
		if(realwrite > m_cap - m_size - m_start)
			realwrite = m_cap - m_size - m_start;
		int32 stop = m_start + m_size;
		if(buf){
			memmove(m_buf + stop, buf, realwrite);
		}
		m_size += realwrite;
		return realwrite;
	}


	int32	frame::clear(){
		int32 ret = m_size;
		m_size = 0;
		m_start = 0;
		return ret;
	}

	int32	loop_buf::firstFrameLen() const{
		assert(m_head);
		return m_head->m_size;
	}

	const uint8* loop_buf::firstFrameData() const{
		assert(m_head);
		return m_head->m_buf + m_head->m_start;
	}

	int32	loop_buf::freeFrameLen() const{
		assert(m_w_pos);
		return m_w_pos->m_cap - m_w_pos->m_size - m_w_pos->m_start;	
	}

	uint8*	loop_buf::freeFrameBuf(){
		assert(m_w_pos);
		return m_w_pos->m_buf + m_w_pos->m_start + m_w_pos->m_size; 
	}

	int32	loop_buf::write(const uint8 *buf, int32 len){
		int32 wlen = 0;
		int32 ret = 0;
		frame*	f = m_w_pos;
		while(len > wlen && f){
			ret = f->write(buf + wlen, len - wlen);
			if(ret <= 0)
				break;
			wlen += ret;
		}
		m_size += wlen;
		return wlen;
	}

	int32	loop_buf::autoResizeWrite(const uint8 *buf, int32 len){
		int32 ret = write(buf, len);
		if(len > ret){
			extend(len - ret);
			write(buf + ret, len -ret);
		}
		return len;
	}

	int32	loop_buf::peek(uint8 *buf, int32 len) const{
		int32 rlen = 0;
		int32 ret = 0;
		frame*  f = m_head;
		frame*	n = f->m_next;
		while(len > rlen && f && f != m_w_pos->m_next){
			ret = f->peek(buf + rlen, len - rlen);	
			rlen += ret;
			f = n;
		}	
		return rlen;
	}

	int32	loop_buf::read(uint8 *buf, int32 len){
		int32 rlen = 0;
		int32 ret = 0;
		frame*  f = m_head;
		frame*  n = f->m_next;
		while(len > rlen && f && f != m_w_pos->m_next){
			n = f->m_next;
			ret = f->read(buf + rlen, len - rlen);	
			rlen += ret;
			if(!f->m_size){
				m_head = f->m_next;
				m_tail->m_next = f;
				m_tail = f;
				f->m_next = NULL;
			}
			f = n;
		}	
		m_size -= rlen;
		return rlen;

	}

	loop_buf::loop_buf(){
		frame* f = allocFrame(1024); 
		m_head = f;
		m_w_pos = f;
		m_tail = f;
		m_size = 0;
	}

	frame*	loop_buf::constructFrame(uint8* buf, int32 len){
		frame* f = (frame*)buf;
		f->m_size = 0;
		f->m_start = 0;
		f->m_next = NULL;	
		f->m_cap = len - sizeof(frame);
		f->m_buf = buf += sizeof(frame);
		return f;	
	}


	int32	loop_buf::alignSize(int32 l){
		return l + DEFAUTLT_ALIGN_SIZE - l % DEFAUTLT_ALIGN_SIZE;
	}

	frame*	loop_buf::allocFrame(int32 l){
		int32 len = alignSize(l);
		uint8* buf = new uint8[len];
		frame* f = constructFrame(buf, len);
		return f;
	}	


	int32	loop_buf::extend(int32 sz){
		frame* f = allocFrame(sz);
		m_tail->m_next = f;
		m_tail = f;
		if(m_w_pos->m_size == m_w_pos->m_cap)
			m_w_pos = m_w_pos->m_next;
		return f->m_cap;
	}	

	int32	loop_buf::clear(){
		frame* f = m_head;
		for(; f; f = f->m_next)	
			f->clear();
		int32 ret = m_size;
		m_size = 0;
		return ret;
	}

	loop_buf::~loop_buf(){
		frame* f = m_head;
		frame* n = f->m_next;
		for(; f; f = n){
			n = f->m_next;	
			delete (uint8*)f;	
		}	
		m_head = NULL;
		m_tail = NULL;
		m_w_pos = NULL;
		m_size = 0;
	}
};

