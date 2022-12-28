# How to build project 4 in M1

1. `git clone https://github.com/JaechanAn/postgres-samsung.git`
2. `cd postgres-samsung`
3. `git submodule init`
4. `git submodule update`
5. `Modify scripts/install_sysbench.sh as below`

```
#!/bin/bash

# Parse parameters
for i in "$@"
do
  case $i in
    --sysbench-base-dir=*)
      SYSBENCH_BASE_DIR="${i#*=}"
      shift
      ;;
    
    --pgsql-install-dir=*)
      INSTALL_DIR="${i#*=}"
      shift
      ;;

    *)
      # unknown option
      ;;
  esac
done

cd ${SYSBENCH_BASE_DIR}

make clean -j4 --silent

./autogen.sh
./configure --without-mysql --with-pgsql --with-pgsql-includes=${INSTALL_DIR}/include --with-pgsql-libs=${INSTALL_DIR}/lib --silent
make -j4 --silent
```

6. Modify run.sh:154 as below

```
--sysbench-base-dir=${SYSBENCH_DIR} \
--pgsql-install-dir=${INSTALL_DIR}
```

7. Modify run_sysbench.sh:79 as below

```
--sysbench-base-dir=${SYSBENCH_DIR} \
--pgsql-install-dir=${BASE_DIR}/pgsql
```

8. Remove the directory sysbench/third_party/luajit

using `rm -rf sysbench/third_party/luajit/luajit`

8. `cd sysbench/third_party/luajit`
9. `git clone https://github.com/LuaJIT/LuaJIT.git luajit`
10. Comment out the Makefile in <u>sysbench/third_party/luajit/luajit/src/Makefile:319~321</u>

```
# ifeq (,$(MACOSX_DEPLOYMENT_TARGET))
#   $(error missing: export MACOSX_DEPLOYMENT_TARGET=XX.YY)
# endif
```

11. Modify postgres/src/backend/utils/init/globals.c:21 as below (When in **border-collie** branch)

```
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
```

