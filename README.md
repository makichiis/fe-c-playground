# fe-c-playground

### Building 
```sh
$ git clone --recurse-submodules -j3 https://github.com/makichiis/fe-c-playground
$ cd fe-c-playground
$ ./run 
```

### Contribution/Code Base Guide (WIP)
I'm still undecided on how to structure the `./run` command suite, but the top-level 
interface should remain roughly the same.

I personally use clangd for syntax highlighting/code hints/linting/etc., and so I've
provided wrappers to bootstrap the configurations necessary for .clangd to properly 
parse the codebase, as well as a small .clangd file which has a few tweaks to work
around some issues relating to the fact that I use GCC instead of clang. **Using GCC 
is highly recommended, and will probably be required at some point.**

If you just want to configure cmake for syntax highlighting to work out of the box, 
run `./run configure`, which configures all targets, including the .clangd 
pseudo-target.

#### Valgrind 
For testing memory, call `./run valgrind [VALGRIND ARGS]...`. 
> Note: There are a bunch of memory loss/leaking errors relating to GLFW and CGLM. I'm 
still attempting to sift through them and possibly get rid of pointless error logs.

#### Sanitize 
For some reason, libasan seems to work horribly with this codebase, crashing, segfaulting,
or halting in non-determinsitic intervals. Until I figure out why, there will be no 
support for `-fsanitize`.


