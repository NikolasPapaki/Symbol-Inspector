This project contains a tool that print information about symbols used by an ELF binary.
This tool is able to work with both stripped and unstripped binaries.

ELF executable binaries contain symbols that map to particular addresses. These symbolic
names are easier to be handled compared to hexadecimal memory addresses. A binary stores
all symbols used by the main executable in a section called .symtab (from symbol table). The
section contains information about each symbol, while the symbol names are stored in another
section called .strtab (from string table).

Additionally, executables often call functions located in shared libraries. These external symbols
are used by the dynamic loader for resolving external functions. These symbols are not stored
in .symtab, but in .dynsym, which is the dynamic symbol table. Their actual names are stored in
the .dynstr section.
