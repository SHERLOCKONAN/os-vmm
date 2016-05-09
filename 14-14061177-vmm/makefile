vmm:vmm.c vmm.h response request
	cc -o vmm vmm.c vmm.h
response: response.c vmm.h
	cc -o response response.c vmm.h
request: request.c vmm.h
	cc -o request request.c vmm.h
clean:
	rm vmm response
