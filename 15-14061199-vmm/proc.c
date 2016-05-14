#include "proc.h"

#include <malloc.h>
#include "page_table.h"
#include <stdlib.h>
#include "vmm.h"

proc_info proc_infos;
extern uchar space[BLOCK_SUM & PAGE_SIZE];

FILE* getFile(pid_t pid)
{
	proc_info t = proc_infos;
	if(t == NULL)
		return NULL;
	while(t){
		if(t->pid == pid)
			break;
		t=t->next;
	}
	if(t)
		return t->file;
	return NULL;
}

int addProc(pid_t pid)
{
	int i;
	proc_info t = proc_infos;
	char filename[20];
	while(t){
		if(t->pid == pid)
			return 1;
		t = t->next;
	}
	t = (proc_info)malloc(sizeof(struct _proc_info));
	t->pid = pid;
	t->next = proc_infos;
	proc_infos = t;
	sprintf(filename,"%d_storage",pid);
	t->file = fopen(filename,"w+");
	if(t->file == NULL){
		if(proc_infos == t)
			proc_infos = NULL;
		free(t);
		return -1;
	}
	for(i=0;i<8;++i){
		t->table.items[i].content_s = (ushort)i;
	}
	for(i=0;i<8;++i){
		int j;
		for(j=0;j<8;++j){
			t->tables[i].items[j].content_s = ((rand()&0x7)<<8)|(((i<<3)|j)<<2);
		}
	}
	
	fseek(t->file,0,SEEK_END);
	if(ftell(t->file) < 1024){
		uchar c;
		fseek(t->file,0,SEEK_SET);
		for(i=0;i<1024;++i){
			c = (uchar)rand();
			fwrite(&c,sizeof(c),1,t->file);
		}
		fflush(t->file);
	}

	return 0;
}

page_table getTable(pid_t pid)
{
	proc_info t = proc_infos;
	while(t && t->pid != pid)
		t = t->next;
	if(t == NULL)
		return NULL;
	else
		return &t->table;
}
page_table getTables(pid_t pid)
{
	proc_info t = proc_infos;
	while(t && t->pid != pid)
		t = t->next;
	if(t == NULL)
		return NULL;
	else
		return t->tables;

}
void rmProc(pid_t pid)
{
	proc_info t = proc_infos;
	proc_info p = NULL;
	if(t == NULL)
		return;
	if(t->pid == pid){
		proc_infos = t->next;
		free(t);
	}
	while(t->next && t->next->pid != pid)
		t = t->next;
	if(t->next == NULL)
		return;
	p = t->next;
	t->next = p->next;
	free(p);
}

void deleteTable(page_table table)
{
}

void proc_print(pid_t pid)
{
	int i;
	page_table pt = getTables(pid);
	for(i=0;i<8;++i){
		int j;
		for(j=0;j<8;++j){ int vp,p;
			char r,w,x;
			char l,m;
			l = m =r = w = x = '-';
			ulong64 age;
extern ushort page_storage_page(table_item item);
extern ushort page_page_num(table_item item);
			vp = page_storage_page(&pt->items[j]);
			p = page_page_num(&pt->items[j]);
			age = get_age(page_page_num(&pt->items[j]));
			if(page_access_r(&pt->items[j])){
				r = 'r';
			}
			if(page_access_w(&pt->items[j])){
				w = 'w';
			}
			if(page_access_x(&pt->items[j])){
				x = 'x';
			}
			if(page_loaded(&pt->items[j])){
				l = 'l';
			}
			if(page_modified(&pt->items[j])){
				m = 'm';
			}
			printf("%2d\t%2d\t%c%c%c\t%c%c\t%llu\n",vp,p,r,w,x,m,l,l=='l'?age:0);
		}
		pt++;
	}
}
