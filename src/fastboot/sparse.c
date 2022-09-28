/*
 *
 * Copyright (C) 2022 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fastboot.h"
#include<pthread.h>
#include<sys/mount.h>

typedef struct sparse_header{
	uint32_t magic;
	uint16_t major_version;
	uint16_t minor_version;
	uint16_t file_head_size;
	uint16_t chunk_head_size;
	uint32_t block_size;
	uint32_t total_blocks;
	uint32_t total_chunks;
	uint32_t image_checksum;
}sparse_header;

typedef struct sparse_chunk_header{
	enum chunk_type{
		CHUNK_TYPE_RAW       = 0xCAC1,
		CHUNK_TYPE_FILL      = 0xCAC2,
		CHUNK_TYPE_DONT_CARE = 0xCAC3,
		CHUNK_TYPE_CRC       = 0xCAC4,
	}chunk_type:16;
	uint16_t reserved_1;
	uint32_t chunk_size;
	uint32_t total_size;
}sparse_chunk_header;

static inline bool chunk_type_to_str(enum chunk_type type,const char**str){
	if(!str)return false;
	switch(type){
		case CHUNK_TYPE_RAW:*str="RAW";return true;
		case CHUNK_TYPE_FILL:*str="FILL";return true;
		case CHUNK_TYPE_DONT_CARE:*str="DONT-CARE";return true;
		case CHUNK_TYPE_CRC:*str="CRC";return true;
	}
	return false;
}

int sparse_probe(void*img,size_t len,size_t*real_size){
	char buf[64];
	sparse_header*head=img;
	if(!img||!real_size)ERET(EINVAL);
	if(len<=sizeof(sparse_header))
		EXGOTO(EINVAL,"sparse file to small");
	if(head->magic!=SPARSE_HEADER_MAGIC)
		EXGOTO(EINVAL,"invalid sparse");
	if(head->file_head_size!=sizeof(sparse_header))
		EXGOTO(EINVAL,"sparse head size mismatch");
	if(head->chunk_head_size!=sizeof(sparse_chunk_header))
		EXGOTO(EINVAL,"sparse chunk head size mismatch");
	if(head->block_size<=0||(head->block_size%4)!=0)
		EXGOTO(EINVAL,"invalid sparse block size");
	*real_size=(uint64_t)head->total_blocks*(uint64_t)head->block_size;
	format_size(buf,sizeof(buf),*real_size,1,0);
	DEBUG(
		"sparse version %u.%u block size %u, "
		"%u blocks, %u chunks, data size %zu (%s)\n",
		head->major_version,head->minor_version,head->block_size,
		head->total_blocks,head->total_chunks,*real_size,buf
	);
	return 0;
	fail:return -1;
}

int sparse_parse(
	sparse_write_cb cb,
	void*img,
	size_t img_len,
	size_t disk_size,
	void*data,
	size_t*progress
){
	char buf[64];
	uint32_t*fb=NULL,v,ch;
	const char*name=NULL;
	size_t chunk_size,qs,prog=0;
	size_t real_size,blocks=0,off=0;
	sparse_header*head=img;
	sparse_chunk_header*chunk=NULL;
	errno=0;
	if(!cb||!img)ERET(EINVAL);
	if(sparse_probe(img,img_len,&real_size)<0)EXRET(EINVAL);
	if(real_size>disk_size)EXGOTO(ENOSPC,"sparse file too large");
	if(!(fb=malloc(head->block_size)))
		EXGOTO(ENOMEM,"alloc block size failed");
	memset(fb,0,head->block_size);
	off+=sizeof(sparse_header);
	for(ch=0;ch<head->total_chunks;ch++){
		size_t cs=(uint64_t)blocks*(uint64_t)head->block_size;
		if(cs>=disk_size)EXGOTO(ENOSPC,"size too large %zu >= %zu",cs,disk_size);
		chunk=img+off,off+=sizeof(sparse_chunk_header);
		if(off>img_len)EXGOTO(EINVAL,"unexpected EOF in chunks");
		if(!chunk_type_to_str(chunk->chunk_type,&name))
			EXGOTO(EINVAL,"unknown chunk type %d",chunk->chunk_type);
		chunk_size=(uint64_t)head->block_size*(uint64_t)chunk->chunk_size;
		format_size(buf,sizeof(buf),chunk_size,1,0);
		DEBUG(
			"chunk #%u: type %s, size %u, "
			"total size %u, data size %zu (%s)\n",
			ch,name,chunk->chunk_size,
			chunk->total_size,chunk_size,buf
		);
		if(cs+chunk_size>disk_size)EXGOTO(ENOSPC,"size too large");
		prog=cs;
		if(progress)*progress=prog;
		switch(chunk->chunk_type){
                        case CHUNK_TYPE_RAW:
				qs=sizeof(sparse_chunk_header)+chunk_size;
				if((size_t)chunk->total_size!=qs)EXGOTO(
					EINVAL,"invalid chunk size of RAW %d != %zu",
					chunk->total_size,qs
				);
				if(off+chunk_size>img_len)EXGOTO(EINVAL,"unexpected EOF in raw chunk");
				if(cb(cs,chunk_size,img+off,data)<0)goto fail;
				blocks+=chunk->chunk_size;
				if(blocks>head->total_blocks)EXGOTO(
					EINVAL,"blocks out of range %zu > %d",
					blocks,head->total_blocks
				);
				off+=chunk_size;

			break;
			case CHUNK_TYPE_FILL:
				qs=sizeof(sparse_chunk_header)+sizeof(uint32_t);
				if((size_t)chunk->total_size!=qs)EXGOTO(
					EINVAL,"invalid chunk size of FILL %d != %zu",
					chunk->total_size,qs
				);
				if(off+sizeof(uint32_t)>img_len)EXGOTO(EINVAL,"unexpected EOF in raw chunk");
				if(cb(cs,0,NULL,data)<0)goto fail;
				v=*(uint32_t*)(img+off),off+=sizeof(uint32_t);
				for(uint32_t i=0;i<head->block_size/sizeof(v);i++)fb[i]=v;
				for(uint32_t i=0;i<chunk->chunk_size;i++){
					if(cs+head->block_size>disk_size)
						EXGOTO(ENOSPC,"size too large");
					if(cb(-1,head->block_size,fb,data)<0)goto fail;
					prog+=head->block_size,blocks++;
					if(progress)*progress=prog;
				}
			break;
			case CHUNK_TYPE_DONT_CARE:
				qs=sizeof(sparse_chunk_header);
				if((size_t)chunk->total_size!=qs)EXGOTO(
					EINVAL,"invalid chunk size of DONT-CARE %d != %zu",
					chunk->total_size,qs
				);
				blocks+=chunk->chunk_size;
				if(blocks>head->total_blocks)EXGOTO(
					EINVAL,"blocks out of range %zu > %d",
					blocks,head->total_blocks
				);
			break;
			case CHUNK_TYPE_CRC:
				qs=sizeof(sparse_chunk_header);
				if((size_t)chunk->total_size!=qs)EXGOTO(
					EINVAL,"invalid chunk size of DONT-CARE %d != %zu",
					chunk->total_size,qs
				);
				blocks+=chunk->chunk_size;
				if(blocks>head->total_blocks)EXGOTO(
					EINVAL,"blocks out of range %zu > %d",
					blocks,head->total_blocks
				);
				if(off+chunk_size>img_len)
					EXGOTO(EINVAL,"unexpected EOF in crc chunk");
				off+=chunk_size;
			break;
		}
	}
	DEBUG("write %u chunks %zu blocks\n",ch,blocks);
	if(blocks!=head->total_blocks||ch!=head->total_chunks)
		EXGOTO(EINVAL,"write blocks or chunks mismatch");
	free(fb);
	return 0;
	fail:
	if(fb)free(fb);
	return -1;
}
