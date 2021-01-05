//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2021/1/5.
//===----------------------------------------------------------------------===//
// ElfReader for Obfuscated so file
//===----------------------------------------------------------------------===//
#ifndef SOFIXER_OBELFREADER_H
#define SOFIXER_OBELFREADER_H

#include "ElfReader.h"

class ObElfReader: public ElfReader {
public:
    ~ObElfReader() override;
    // the phdr informaiton in dumped so may be incorrect,
    // try to fix it
    void FixDumpSoPhdr();

    bool Load() override;
    bool LoadDynamicSection();

    void setDumpSoBaseAddr(Elf_Addr base) { dump_so_base_ = base; }

    void setBaseSoName(const char* name) {
        baseso_ = name;
    }

    void GetDynamicSection(Elf_Dyn** dynamic, size_t* dynamic_count, Elf_Word* dynamic_flags) override;


private:
    Elf_Addr dump_so_base_ = 0;

    const char* baseso_ = nullptr;

    void* dynamic_sections_ = nullptr;
    size_t dynamic_count_;
    Elf_Word dynamic_flags_;

};


#endif //SOFIXER_OBELFREADER_H