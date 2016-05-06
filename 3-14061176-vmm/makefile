vmm: vmm.c vmm.h request.c response.c error.c request
	cc -g -o vmm vmm.c vmm.h request.c response.c error.c -DORZ -Wformat=0
request: gen_request.c error.c
	cc -g -o request gen_request.c error.c
clean:
	rm vmm request
