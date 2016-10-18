llvm/build/Makefile :
	rm -rf llvm/build
	mkdir -p llvm/build
	cd llvm/build && ../configure --enable-optimized --with-binutils-include=/usr/include

llvm: llvm/build/Makefile
	cd llvm/build && $(MAKE) && sudo make install

minisat/build/Makefile:
	rm -rf minisat/build
	mkdir -p minisat/build
	cd minisat/build && cmake -DCMAKE_INSTALL_PREFIX=/usr/ ..

minisat: minisat/build/Makefile
	cd minisat/build && $(MAKE) && sudo make install

stp/build/Makefile:
	rm -rf stp/build
	mkdir -p stp/build
	cd stp/build && cmake -DSTATICCOMPILE=ON -DCMAKE_INSTALL_PREFIX=/usr/ ../

stp: stp/build/Makefile
	cd stp/build && $(MAKE) && sudo make install

z3/build/build.ninja:
	rm -rf z3/build
	cd z3 && git clean -fx && contrib/cmake/bootstrap.py create && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUSE_OPENMP=ON -DCMAKE_INSTALL_PREFIX=/usr -GNinja ..

z3: z3/build/build.ninja
	cd z3/build && ninja && sudo ninja install

uclibc/ready:
	rm -f uclibc/ready
	cd uclibc &&  ./configure --make-llvm-lib && touch ready

uclibc: uclibc/ready
	cd uclibc && $(MAKE)

klee/build/Makefile:
	rm -rf klee/build
	mkdir -p klee/build
	cd klee/build && ../configure "--with-z3=/usr" "--with-uclibc=${PWD}/uclibc" --enable-posix-runtime

klee: klee/build/Makefile
	cd klee/build && $(MAKE) DISABLE_ASSERTIONS=0 ENABLE_OPTIMIZED=1 ENABLE_SHARED=0 || $(MAKE) -j1 DISABLE_ASSERTIONS=0 ENABLE_OPTIMIZED=1 ENABLE_SHARED=0

dist-clean:
	rm -rf llvm/build
	rm -rf minisat/build
	rm -rf stp/build
	cd z3 && rm -rf build && git clean -fx
	cd uclibc && $(MAKE) clean
	rm -f uclibc/ready
	rm -rf klee/build
	true

clean:
	cd klee/build && $(MAKE) clean
	cd uclibc && $(MAKE) clean
	cd stp/build && $(MAKE) clean
	cd z3/build && ninja clean
	cd minisat/build && $(MAKE) clean
	cd llvm/build && $(MAKE) clean
	true

check:
	cd llvm/build && $(MAKE) check && cd - && \
	cd stp/build && $(MAKE) check && cd - && \
	cd z3/build && ninja check && cd - && \
	cd klee/build && $(MAKE) check

all:
	$(MAKE) llvm && $(MAKE) minisat && $(MAKE) stp && $(MAKE) z3 && $(MAKE) uclibc && $(MAKE) klee

fp_bench_clone:
	git clone https://github.com/delcypher/fp-bench && cd fp-bench && git checkout c198aab222cd95616511f605f8e345c6dd44d87b
	cd fp-bench/benchmarks/c/ && git clone https://github.com/delcypher/fp-benchmarks-imperial.git imperial && \
		cd imperial && git checkout 098085a9eb419930fb982eca5be4d3fc7a0d8655
	cd fp-bench/benchmarks/c/ && git clone -b forked https://github.com/delcypher/fp-benchmarks-aachen.git aachen && \
		cd aachen && git checkout e7b67b70d91b1eee30bbbed9df76cbf2ec2cd3c9

fp_bench_build_O0:
	mkdir -p build_O0
	cd build_O0 && \
		CC=/home/user/whole-program-llvm/wllvm \
		CXX=/home/user/whole-program-llvm/wllvm++ \
		KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/ \
		KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/Release+Asserts/lib/ \
		LLVM_COMPILER=clang \
		PATH="$$PATH:/home/user/whole-program-llvm" \
		cmake \
		-DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_IMPERIAL_BENCHMARKS=OFF \
		-DBUILD_AACHEN_BENCHMARKS=ON \
		../fp-bench/
	cd build_O0 && LLVM_COMPILER=clang make create-augmented-spec-file-list
	cd build_O0 && ../fp-bench/svcb/tools/filter-augmented-spec-list.py --categories issta_2017 -- augmented_spec_files.txt > issta_augmented_spec_files.txt
	cd build_O0 && ../fp-bench/svcb/tools/svcb-emit-klee-runner-invocation-info.py issta_augmented_spec_files.txt -o issta_invocation_info.yml

fp_bench_build_O2:
	mkdir -p build_O2
	cd build_O2 && \
		CC=/home/user/whole-program-llvm/wllvm \
		CXX=/home/user/whole-program-llvm/wllvm++ \
		KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/ \
		KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/Release+Asserts/lib/ \
		LLVM_COMPILER=clang \
		PATH="$$PATH:/home/user/whole-program-llvm" \
		cmake \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DBUILD_IMPERIAL_BENCHMARKS=ON \
		-DBUILD_AACHEN_BENCHMARKS=OFF \
		../fp-bench/
	cd build_O2 && LLVM_COMPILER=clang make create-augmented-spec-file-list
	cd build_O2 && ../fp-bench/svcb/tools/filter-augmented-spec-list.py --categories issta_2017 -- augmented_spec_files.txt > issta_augmented_spec_files.txt
	cd build_O2 && ../fp-bench/svcb/tools/svcb-emit-klee-runner-invocation-info.py issta_augmented_spec_files.txt -o issta_invocation_info.yml


pull:
	cd llvm && git pull
	cd llvm/tools/clang && git pull
	#cd llvm/projects/compiler-rt && git pull
	cd llvm/projects/test-suite && git pull
	cd minisat && git pull
	cd stp && git pull --recurse-submodules && git submodule update --recursive
	cd z3 && git pull
	cd uclibc && git pull
	cd klee && git pull
	cd whole-program-llvm && git pull

gc:
	cd llvm && git gc --aggressive
	cd llvm/tools/clang && git gc --aggressive
	#cd llvm/projects/compiler-rt && git gc --aggressive
	cd llvm/projects/test-suite && git gc --aggressive
	cd minisat && git gc --aggressive
	cd stp && git gc --aggressive && git submodule foreach --recursive 'git gc --aggressive'
	cd z3 && git gc --aggressive
	cd uclibc && git gc --aggressive
	cd klee && git gc --aggressive
	cd whole-program-llvm && git gc --agressive

unshallow:
	cd llvm && git fetch --unshallow
	cd llvm/tools/clang && git fetch --unshallow
	#cd llvm/projects/compiler-rt && git fetch --unshallow
	cd llvm/projects/test-suite && git fetch --unshallow
	cd minisat && git fetch --unshallow
	cd stp && git fetch --unshallow
	cd z3 && git fetch --unshallow
	cd uclibc && git fetch --unshallow
	cd klee && git fetch --unshallow
	cd whole-program-llvm && git fetch --unshallow

.PHONY : all pull gc unshallow clean dist-clean check llvm minisat stp z3 uclibc klee fp_bench_clone fp_bench_build_O0 fp_bench_build_O2
.DEFAULT_GOAL := all
