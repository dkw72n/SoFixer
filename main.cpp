#include <iostream>
#include "ObElfReader.h"
#include "ElfRebuilder.h"
#include "FDebug.h"
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __SO64__
#define TARGET_NAME "SoFixer64"
#else
#define TARGET_NAME "SoFixer32"
#endif


#define DUMP_CPP_LIB
#include "dump.cpp"

const char* short_options = "hdm:s:o:b:p:l:";
const struct option long_options[] = {
	{"pid", 1, NULL, 'p'},
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"memso", 1, NULL, 'm'},
        {"source", 1, NULL, 's'},
        {"baseso", 1, NULL, 'b'},
        {"output", 1, NULL, 'o'},
        {"last", 1, NULL, 'l'},
        {nullptr, 0, nullptr, 0}
};
void useage();


bool main_loop(int argc, char* argv[]) {
    int c;
    int pid = 0;
    unsigned long long left,right = 0;
    ObElfReader elf_reader;

    std::string source, output, baseso;
    while((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (c) {
            case 'd':
                FLOGI("Use debug mode");
                break;
            case 's':
                source = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'b':
                baseso = optarg;
                break;
            case 'm': {
                auto is16Bit = [](const char* c) {
                    auto len = strlen(c);
                    if(len > 2) {
                        if(c[0] == '0' & c[1] == 'x') return true;
                    }
                    bool is10bit = true;
                    for(auto i = 0; i < len; i++) {
                        if((c[i] > 'a' && c[i] < 'f') ||
                           (c[i] > 'A' && c[i] < 'F')) {
                            is10bit = false;
                        }
                    }
                    return !is10bit;
                };
#ifndef __SO64__
                auto base = strtoul(optarg, 0, is16Bit(optarg) ? 16: 10);
#else
                auto base = strtoull(optarg, 0, is16Bit(optarg) ? 16: 10);
#endif
                left = base;
                elf_reader.setDumpSoBaseAddr(base);
            }
                break;
	    case 'p':
                pid = atoi(optarg);
	        break;
	    case 'l':
        right = strtoull(optarg, NULL, 0);
		    break;
      default:
        return false;
      }
    }

#define USE_PROCESS_VM_READV
#ifndef USE_PROCESS_VM_READV
    if (pid){
            char buf[4096];
            int fd = -1, fd2 = -1;
            int should_seek = 1;
            FLOGE("dump begin");
            kill(pid, SIGSTOP);
            fd = open(source.c_str(), O_RDONLY);
            if (fd < 0){
                    FLOGE("faile to open source");
                    goto exit1;
            }
            fd2 = open("/data/local/tmp/", O_WRONLY|O_TMPFILE, 0660);
            for(unsigned long long p = left; p < right; p += 4096){
                    if (should_seek) lseek64(fd, p, SEEK_SET);
                    if (read(fd, buf, 4096) == 4096){
                            should_seek = 0;
                    } else {
                            should_seek = 1;
                    }
                    write(fd2, buf, 4096);
            }      
            sprintf(buf, "/proc/self/fd/%d", fd2);
            source = buf;
            FLOGE("source = %s", source.c_str());
            FLOGE("dump end");
exit1:
            if (fd > 0) close(fd);
            // if (fd2 > 0) close(fd2);
            kill(pid, SIGCONT);
    }

#else

    if (pid){
            FLOGE("dump begin with process_vm_read");
            kill(pid, SIGSTOP);
	    const char* buf = dump_pid(pid, left, right);
	    if (buf){
	        source = buf;
            	FLOGE("source = %s", source.c_str());
            	FLOGE("dump end");
	    } else {
		FLOGE("dump failed");
	    }
            kill(pid, SIGCONT);
    }

#endif
    auto file = fopen(source.c_str(), "rb");
    if(nullptr == file) {
        FLOGE("source so file cannot found!!!");
        return false;
    }
#ifdef __LARGE64_FILES
    auto fd = file->_file;
#else
    auto fd = fileno(file);
#endif


    FLOGI("start to rebuild elf file");
    if (!elf_reader.setSource(source.c_str())) {
        FLOGE("unable to open source file");
        return false;
    }
    if (!baseso.empty()) {
        elf_reader.setBaseSoName(baseso.c_str());
    }

    if(!elf_reader.Load()) {
        FLOGE("source so file is invalid");
        return false;
    }

    ElfRebuilder elf_rebuilder(&elf_reader);
    if(!elf_rebuilder.Rebuild()) {
        FLOGE("error occured in rebuilding elf file");
        return false;
    }
    fclose(file);

    if (!output.empty()) {
        file = fopen(output.c_str(), "wb+");
        if(nullptr == file) {
            FLOGE("output so file cannot write !!!");
            return false;
        }
        fwrite(elf_rebuilder.getRebuildData(), 1, elf_rebuilder.getRebuildSize(),  file);
        fclose(file);
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (main_loop(argc, argv)) {
        FLOGI("Done!!!");
        return 0;
    }
    useage();
    return -1;
}

void useage() {
    FLOGI(TARGET_NAME "v2.1 author F8LEFT(currwin)");
    FLOGI("Useage: SoFixer <option(s)> -s sourcefile -o generatefile");
    FLOGI(" try rebuild shdr with phdr");
    FLOGI(" Options are:");
    FLOGI("  -p --pid");
    FLOGI("  -d --debug                                 Show debug info");
    FLOGI("  -m --memso memBaseAddr(16bit format)       the memory address x which the source so is dump from");
    FLOGI("  -s --source sourceFilePath                 Source file path");
    FLOGI("  -b --baseso baseFilePath                   Original so file path.(used to get base information)(experimental)");
    FLOGI("  -o --output generateFilePath               Generate file path");
    FLOGI("  -h --help                                  Display this information");
}
