/**
* https://sourceware.org/binutils/docs/binutils/nm.html
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libelf.h>
#include <gelf.h>

#define DIE(...)                  \
  do                              \
  {                               \
    fprintf(stderr, __VA_ARGS__); \
    fputc('\n', stderr);          \
    exit(EXIT_FAILURE);           \
  } while (0)


char Decode_Type(GElf_Shdr shdr, GElf_Sym sym)
{
  char c;

  if (ELF64_ST_BIND(sym.st_info) == STB_GNU_UNIQUE) // symbol is unique
    c = 'u';
  else if (ELF64_ST_BIND(sym.st_info) == STB_WEAK) // symbol is Weak
  {
    c = 'W';
    if (sym.st_shndx == SHN_UNDEF) // symbol is weak and undefined
      c = 'w';
  }
  else if (ELF64_ST_BIND(sym.st_info) == STB_WEAK && ELF64_ST_TYPE(sym.st_info) == STT_OBJECT) // symbol is weak and data object
  {
    c = 'V';
    if (sym.st_shndx == SHN_UNDEF) // symbol is undefined , weak and data object
      c = 'v';
  }
  else if (sym.st_shndx == SHN_UNDEF) // symbol is undefined
    c = 'U';
  else{
    
    if (sym.st_shndx == SHN_ABS) // symbol value is absolute
      c = 'A';
    else if (sym.st_shndx == SHN_COMMON) // symbol is common
      c = 'C';
    else if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_ALLOC | SHF_WRITE))  // symbol in bss section
      c = 'B';
    else if (shdr.sh_type == SHT_PROGBITS && shdr.sh_flags == SHF_ALLOC) // symbol is in a read only data section
      c = 'R';
    else{ 
      if (shdr.sh_type == SHT_PROGBITS && shdr.sh_flags == (SHF_ALLOC | SHF_WRITE)) // symbol is in the initialized data section.
        c = 'D';
      else if (shdr.sh_type == SHT_PROGBITS && shdr.sh_flags == (SHF_ALLOC | SHF_EXECINSTR)) // symbol is in the text (code) section.
        c = 'T';
      else if (shdr.sh_type == SHT_DYNAMIC) // symbol is dynamic
        c = 'D';
      else
        c = ('t' - 32);  
    }
  }
    

  if (ELF64_ST_BIND(sym.st_info) == STB_LOCAL && c != '?') // symbol has internal reference
    c += 32;

  return (c);
}

char find_type(int index, Elf *elf, GElf_Sym sym)
{
  int i;
  Elf_Scn *scn = NULL;
  GElf_Shdr shdr;

  for (i = 0; i < index; i++)   //Loop until you reach the i-th section
  {
    scn = elf_nextscn(elf, scn); //Get next section

    if (gelf_getshdr(scn, &shdr) != &shdr)    //Get section header
      DIE("(getshdr) %s", elf_errmsg(-1));
  }

  return Decode_Type(shdr, sym);
}

char *find_section(int index, Elf *elf)
{
  int i;
  Elf_Scn *scn = NULL;
  size_t shstrndx;
  GElf_Shdr shdr;

  /* Get section header index. */
  if (elf_getshdrstrndx(elf, &shstrndx) != 0) 
    DIE("(getshdrstrndx) %s", elf_errmsg(-1));

  for (i = 0; i < index; i++)  //Loop until you reach the i-th section
  { 
    scn = elf_nextscn(elf, scn);   //Get next section

    if (gelf_getshdr(scn, &shdr) != &shdr) //Get section header
      DIE("(getshdr) %s", elf_errmsg(-1));
  }

  char *Result = NULL;
  Result = elf_strptr(elf, shstrndx, shdr.sh_name); //Get the name of section

  if (Result == NULL) //If Result is null return a space character 
    return " ";
  else
    return Result;
}

void print_dynamic_table(Elf *elf, Elf_Scn *scn)
{
  Elf_Data *data;
  GElf_Shdr shdr;
  int count = 0;

  size_t shstrndx;

  if (elf_getshdrstrndx(elf, &shstrndx) != 0)
    DIE("(getshdrstrndx) %s", elf_errmsg(-1));

  /* Get the descriptor.  */
  if (gelf_getshdr(scn, &shdr) != &shdr)
    DIE("(getshdr) %s", elf_errmsg(-1));

  data = elf_getdata(scn, NULL);
  count = shdr.sh_size / shdr.sh_entsize;

  fprintf(stderr, "\nPrinting Dynamic table.\n");

  fprintf(stderr, "\nName\t\t\t\t\tType\tValue\t\tRelevant Section\n");
  for (int i = 0; i < count; ++i)
  {
    GElf_Sym sym;
    gelf_getsym(data, i, &sym);
    if (ELF64_ST_TYPE(sym.st_info) == STT_FUNC || ELF64_ST_TYPE(sym.st_info) == STT_OBJECT)
    {
      if (sym.st_value != 0)
        fprintf(stderr, "%-32s\t%c\t0x%010lx\t%s\n", elf_strptr(elf, shdr.sh_link, sym.st_name), find_type(sym.st_shndx, elf, sym), sym.st_value, find_section(sym.st_shndx, elf));
      else
        fprintf(stderr, "%-32s\t%c\t0x%010lx\n", elf_strptr(elf, shdr.sh_link, sym.st_name), find_type(sym.st_shndx, elf, sym), sym.st_value);
    }
  }
}

void print_symbol_table(Elf *elf, Elf_Scn *scn)
{
  Elf_Data *data;
  GElf_Shdr shdr;
  int count = 0;

  size_t shstrndx;

  /* Get section header index. */
  if (elf_getshdrstrndx(elf, &shstrndx) != 0)
    DIE("(getshdrstrndx) %s", elf_errmsg(-1));

  /* Get the descriptor.  */
  if (gelf_getshdr(scn, &shdr) != &shdr)
    DIE("(getshdr) %s", elf_errmsg(-1));

  data = elf_getdata(scn, NULL);
  count = shdr.sh_size / shdr.sh_entsize;

  fprintf(stderr, "\nPrinting symbol table.\n");
  fprintf(stderr, "\nName\t\t\t\t\tType\tValue\t\tRelevant Section\n");

  for (int i = 0; i < count; ++i) // Loop until you process all symbols in symbol table
  {
    GElf_Sym sym;
    gelf_getsym(data, i, &sym);
    if (ELF64_ST_TYPE(sym.st_info) == STT_FUNC || ELF64_ST_TYPE(sym.st_info) == STT_OBJECT)
    {
      if (sym.st_value != 0)
        fprintf(stderr, "%-32s\t%c\t0x%010lx\t%s\n", elf_strptr(elf, shdr.sh_link, sym.st_name), find_type(sym.st_shndx, elf, sym), sym.st_value, find_section(sym.st_shndx, elf));
      else
        fprintf(stderr, "%-32s\t%c\t0x%010lx\n", elf_strptr(elf, shdr.sh_link, sym.st_name), find_type(sym.st_shndx, elf, sym), sym.st_value);
    }
  }
}


void load_file(char *filename)
{

  Elf *elf;
  Elf_Scn *symtab; /* To be used for printing the symbol table.    */
  Elf_Scn *dynsym = NULL; /* To be used for printing the dyncamic table.  */

  /* Initilization.  */
  if (elf_version(EV_CURRENT) == EV_NONE)
    DIE("(version) %s", elf_errmsg(-1));

  int fd = open(filename, O_RDONLY);

  elf = elf_begin(fd, ELF_C_READ, NULL);
  if (!elf)
    DIE("(begin) %s", elf_errmsg(-1));

  /* Loop over sections.  */
  Elf_Scn *scn = NULL;
  GElf_Shdr shdr;
  size_t shstrndx;

  if (elf_getshdrstrndx(elf, &shstrndx) != 0)
    DIE("(getshdrstrndx) %s", elf_errmsg(-1));

  int found = 0; // Flag used to determine if file has symbol table

  while ((scn = elf_nextscn(elf, scn)) != NULL)
  {
    if (gelf_getshdr(scn, &shdr) != &shdr)
      DIE("(getshdr) %s", elf_errmsg(-1));

    /* Locate symbol table.  */
    if (!strcmp(elf_strptr(elf, shstrndx, shdr.sh_name), ".symtab"))
    {
      symtab = scn;
      found = 1;
    }

    /* Locate dynamic table. */
    if (!strcmp(elf_strptr(elf, shstrndx, shdr.sh_name), ".dynsym"))
      dynsym = scn;
  }

  /* Call the functions to print symbol and dynamic tables*/
  if (found == 0)
    fprintf(stderr,"File is stripped and symblol table cant be found\n");
  else
    print_symbol_table(elf, symtab);

  print_dynamic_table(elf, dynsym);
}

int main(int argc, char *argv[])
{

  if (argc < 2)
    DIE("usage: elfloader <filename>");

  load_file(argv[1]);

  return 1;
}
