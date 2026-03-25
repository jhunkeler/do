# do
Minimal make-ish command runner

## Installation

```shell
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
make install
```

## Doing things

Running `do` without arguments will look for a `dofile` in the current directory, and run the `default` target automatically.

**dofile**:
```
default:
    echo "default target executed"
```

__Output__:

```
==> Running target default
default target executed
```

`do` supports targets with dependencies.

```
hello:
    echo "hello world"

default: hello
    echo "default target executed"
```


__Output__:

```
==> Running target hello
hello world
==> Running target default
default target executed
```

You can `include` other `dofile`s as well.

*dofile_hello*:
```
hello:
    echo "hello"
```

*dofile_world*:
```
include dofile_hello

world: hello 
    echo "world"
```

*dofile*:
```
include dofile_world

default: world
    echo "default target executed"
```

__Output__:
```
==> Running target hello
hello
==> Running target world
world
==> Running target default
default target executed
```
