# Windows voice decoder

The client statically links the x86_64 MinGW libraries from these MSYS2 packages.
Their headers are under `deps/include`, the static archives under `deps/lib`,
and the license texts are embedded in the executable as Windows resources.

- [libogg 1.3.6-1](https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-libogg-1.3.6-1-any.pkg.tar.zst)
- [opus 1.6.1-2](https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-opus-1.6.1-2-any.pkg.tar.zst)
- [opusfile 0.12-5](https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-opusfile-0.12-5-any.pkg.tar.zst)
