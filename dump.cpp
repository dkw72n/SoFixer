#include <stdint.h>
#include <sys/uio.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static const char* dump_pid(int pid, uint64_t base, uint64_t end){
	char buf[64];
	int fd = open("/data/local/tmp", O_WRONLY|O_TMPFILE, 0660);
	if (!fd) return NULL;
	struct iovec local, remote;
	local.iov_base = malloc(4096);
	local.iov_len = 4096;
	for(uint64_t cur = base; cur < end; cur += 4096){
		remote.iov_base = (void*)cur;
		remote.iov_len = 4096;
		ssize_t nread = process_vm_readv(pid, &local, 1, &remote, 1, 0);
		if (nread != 4096){
			printf("read error at %p\n", cur);
		}
		if (write(fd, local.iov_base, local.iov_len) != local.iov_len){
			printf("write error at %p\n", cur);
			break;
		}
	}
	sprintf(buf, "/proc/self/fd/%d", fd);
	return strdup(buf);

}

#ifndef DUMP_CPP_LIB
int main(int argc, char** argv){
	if (argc != 4){
		return -1;
	}
	int pid = atoi(argv[1]);
	uint64_t base = strtoull(argv[2], NULL, 0);
	uint64_t end = strtoull(argv[3], NULL, 0);
	if (!(pid && base && end)){
		return -2;
	}
	const char* ret = dump_pid(pid, base, end);
	printf("ret: %s\n", ret);
	return 0;
}
#endif
