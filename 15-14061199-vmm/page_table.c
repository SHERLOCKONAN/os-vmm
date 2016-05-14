#include "page_table.h"
#include "proc.h"
#include "vmm.h"
#include <stdio.h>
#include <string.h>

mem_manage g_mm;
uchar* g_mem_ptr;
ulong age_table[BLOCK_SUM];
extern uchar space[BLOCK_SUM * PAGE_SIZE];
/********Access********/
BOOL page_access_w(table_item item)
{
	if(item == NULL)
		return FALSE;
	if(item->content_s&0x2<<8)
		return TRUE;
	return FALSE;
}

BOOL page_access_r(table_item item)
{
	if(item == NULL)
		return FALSE;
	if(item->content_s&0x4<<8)
		return TRUE;
	return FALSE;
}
BOOL page_access_x(table_item item)
{
	if(item == NULL)
		return FALSE;
	if(item->content_s&0x1<<8)
		return TRUE;
	return FALSE;
}
void set_page_access_w(table_item item, BOOL b)
{
	if(item == NULL)
		return;
	if(b == TRUE)
		item->content_c[0] |= 0x1;
	else if(b == FALSE)
		item->content_c[0] &= ~0x1;
}
void set_page_access_r(table_item item, BOOL b)
{
	if(item == NULL)
		return;
	if(b == TRUE)
		item->content_c[0] |= 0x2;
	else if(b == FALSE)
		item->content_c[0] &= ~0x2;
}
void set_page_access_x(table_item item, BOOL b)
{
	if(item == NULL)
		return;
	if(b == TRUE)
		item->content_c[0] |= 0x4;
	else if(b == FALSE)
		item->content_c[0] &= ~0x4;
}
/********Modify&Load********/
BOOL page_modified(table_item item)
{
	if(item == NULL)
		return FALSE;
	if(item->content_s & 0x2)
		return TRUE;
	return FALSE;
}
BOOL page_loaded(table_item item)
{
	if(item == NULL)
		return FALSE;
	if(item->content_s & 0x1)
		return TRUE;
	return FALSE;
}
/********Infomation********/
//ushort page_offset(table_item item)
//{
//	if(item == NULL)
//		return -1;
//	return item->content_s & 0xf;
//}
ushort page_page_num(table_item item)
{
	if(item == NULL)
		return -1;
	return item->content_s >> 11;
}
ushort page_storage_page(table_item item)
{
	if(item == NULL)
		return -1;
	return (item->content_s >> 2) & 0x3f;
}
/********Operation********/

void set_item_load(table_item item)
{
//	printf("set %hu loaded\n",page_storage_page(item));
	item->content_s |= 0x1;
}
void set_item_modified(table_item item)
{
	//printf("set %lld modified\n",(long long)item);
	item->content_s |= 0x2;
}
void reset_item_load(table_item item)
{
//	printf("reset %d loaded\n",page_storage_page(item));
	item->content_s &= ~0x1;
}
void reset_item_modified(table_item item)
{
	//printf("reset %lld modified\n",(long long)item);
	item->content_s &= ~0x2;
}

void do_page_out_z(pid_t pid, ushort vpage);
void do_page_in_z(pid_t pid, ushort vpage);

ushort pre_load(table_item item)
{
	ushort i;
	ushort num = 0xffff;
	ushort vp = page_storage_page(item);
	for(i= page_storage_page(item) & 0x7;
			i< 32;
				i+= 0x8){
		if(0 == get_pid_by_block_num(i)){
			num = i;
			break;
		}
		else if (num == 0xffff){
			num = i;
		}
		else{
			if(get_age(i) < get_age(num)){
				num = i;
			}
		}
	}
	age(num);
	if(get_pid_by_block_num(num)){
	//	printf("out? %d ",num);
	do_page_out_z(get_pid_by_block_num(num),get_vpage(num));
	//	printf("out!");
	}
	item->content_s = (item->content_s & 0x7ff)|num<<11;
//	item->content_s |= 0x1;
//	item->content_s &= ~0x2;
	return num;
}
page_table prepare_do(pid_t pid)
{
	page_table table = getTable(pid);
	table_item item = &table->items[7];
	if(!page_loaded(item)){
		printf("缺页:%d -> ",page_storage_page(item));
		printf("%d\n",pre_load(item));
		//pre_load(item);
		do_page_in(pid,item);
		memcpy(space+(page_page_num(item)<<4),table,sizeof(struct _page_table));
	}
	age(page_page_num(item));
	return (page_table)(space + (page_page_num(item) << 4));
}

table_item get_item(pid_t pid, ushort address)
{
	page_table table = getTable(pid);
	if(table == NULL)
		return NULL;
	page_table pt = getTables(pid);
	table_item item = &table->items[address>>7];
	item = &pt[item->content_s].items[(address>>4)&0x7];
	return item;
}
ERROR_CODE read_from(pid_t pid, ushort address, uchar* res)
{
	table_item item ;
	ushort a_address;
	if(pid == 0){
		if(address >= 512){
			return ERROR_OVER_BOUNDARY;
		}
		*res = space[address];
		return ERROR_OK;
	}
	item = get_item(pid,address);
	if(address >= 1024){
		return ERROR_OVER_BOUNDARY;
	}
	if(item == NULL){
		return ERROR_PROC_INVALID;
	}
	if(!page_loaded(item)){
		printf("lose page %d\n",page_storage_page(item));
		do_page_in_z(pid,address>>4);
	}
	if(!page_access_r(item)){
		return ERROR_READ_DENY;
	}
	a_address = (page_page_num(item)<<4) | (address&0xf);
	*res = space[a_address];
	age(a_address>>4);
	printf("%d -> %d\n",address,a_address);
	return ERROR_OK;

}
ERROR_CODE execute_at(pid_t pid, ushort address)
{
	table_item item = get_item(pid,address);
	ushort a_address;
	if(!page_loaded(item)){
		printf("lose page %d\n",page_storage_page(item));
		do_page_in_z(pid,address>>4);
	}
	if(!page_access_x(item)){
		return ERROR_READ_DENY;
	}
	a_address = (page_page_num(item)<<4) | (address&0xf);
	printf("Exe %d @ %d\n",space[a_address],a_address);
	age(a_address>>4);
	printf("%d -> %d\n",address,a_address);
	return ERROR_OK;

}
ERROR_CODE write_to(pid_t pid, ushort address, uchar data)
{
	table_item item = get_item(pid,address);
	ushort a_address;
	if(!page_loaded(item)){
		do_page_in_z(pid,address>>4);
	}
	if(!page_access_w(item)){
		return ERROR_WRITE_DENY;
	}
	set_item_modified(item);
	a_address = (page_page_num(item)<<4) | (address&0xf);
	age(a_address>>4);
	space[a_address] = data;
	printf("%d -> %d\n",address,a_address);
	return ERROR_OK;
}


void mark_modified(pid_t pid, ushort vpage)
{
	page_table table = getTable(pid);
	if(!page_loaded(&table->items[7])){
		pre_load(&table->items[7]);
		do_page_in(pid,&table->items[7]);
	}
	table = (page_table)(space + (page_page_num(&table->items[7])<<4));
	memcpy(table,getTable(pid),PAGE_SIZE);
}



void do_page_out(table_item item)
{
	ushort block_num = page_page_num(item);
	pid_t pid = get_pid_by_block_num(block_num);
//	ushort vpage = get_vpage(block_num);
//	page_table table = getTable(pid);
	store_page(block_num, getFile(pid), page_storage_page(item)<<OFFSET_BIT_NUM);

	reset_item_modified(item);
	reset_item_load(item);
//	item->content_s &= ~0x3;
//	set_mm_ctrl(block_num, 0);
}

ERROR_CODE do_page_in(pid_t pid, table_item item)
{
	FILE* file = getFile(pid);
	ushort block_num = page_page_num(item);
	load_page(block_num,file,page_storage_page(item)<<OFFSET_BIT_NUM);
//	set_mm_ctrl(block_num, pid);
	return ERROR_OK;
}

void do_page_in_a(pid_t pid);
void do_page_in_b(pid_t pid, ushort vpage);
void do_page_in_c(pid_t pid, ushort vpage);

//void do_page_in_a(pid_t pid)
//{
//	FILE* file = getFile(pid);
//	page_table table = getTable(pid);
//	table_item item = &table->items[7];
//	if(!page_loaded(&table->items[7])){
//		pre_load(&table->items[7]);
//	}
//}
void do_page_in_z(pid_t pid, ushort vpage)
{
	extern ushort get_pre(ushort);
	table_item item = get_item(pid,vpage<<4);
	ushort pr = get_pre(vpage);
	//printf("\tload %d to %d\n",vpage,pr);
	if(get_pid_by_block_num(pr) != 0){
		do_page_out_z(get_pid_by_block_num(pr),get_vpage(pr));
	}
	fseek(getFile(pid),page_storage_page(item)<<4,SEEK_SET);
	fread(space+(pr<<4),PAGE_SIZE,1,getFile(pid));
	set_item_load(item);
	set_mm_ctrl(pr,pid,vpage);
	item->content_s &= ~(0x1f<<11);
	item->content_s |= pr<<11;
}
void do_page_out_z(pid_t pid, ushort vpage)
{
	table_item item = get_item(pid,vpage<<4);
	fseek(getFile(pid),page_storage_page(item)<<4,SEEK_SET);
	fwrite(space+(page_page_num(item)<<4),PAGE_SIZE,1,getFile(pid));
	reset_item_load(item);
	reset_item_modified(item);
}

