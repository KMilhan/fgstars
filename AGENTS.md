Use $(nproc) when compiling
Run UI tests non-headless mode, as headless mode is currently unstable
Treat all compiler warnings as errors
For this repository, all commits must use gitmoji prefixes regardless of upstream conventions
For this repository, work directly on master by default and push completed changes to origin/master immediately unless the user explicitly asks for a different branch workflow
Prefer C++26 language and standard library features in new or touched C++ code whenever they materially reduce code size, state duplication, or incidental complexity
Prefer standard library value and math types such as std::complex, std::optional, std::expected, std::span, ranges, and related modern declarations over handwritten helper structs or verbose legacy patterns when they make the code shorter and clearer
Use newer C++26 declarations and initialization forms aggressively when they improve readability, but do not force them where they would obscure intent or fight Qt/KDE APIs
