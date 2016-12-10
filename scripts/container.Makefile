NPROC := $(shell nproc)

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

gtest:
	wget https://github.com/google/googletest/archive/release-1.7.0.zip
	unzip release-1.7.0.zip

klee/build/Makefile: gtest
	rm -rf klee/build
	mkdir -p klee/build
	cd klee/build && cmake \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DENABLE_KLEE_ASSERTS=ON \
		-DENABLE_KLEE_UCLIBC=ON \
		-DKLEE_UCLIBC_PATH=${PWD}/uclibc \
		-DENABLE_POSIX_RUNTIME=ON \
		-DENABLE_SOLVER_Z3=ON \
		-DENABLE_TCMALLOC=OFF \
		-DGTEST_SRC_DIR=${PWD}/googletest-release-1.7.0 \
		../

klee: klee/build/Makefile
	cd klee/build && $(MAKE) -j$(NPROC) && make check

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

FPBENCH_URL := https://github.com/delcypher/fp-bench.git
FPBENCH_COMMIT := 440b9a2402acfe5bd110f871c208808289606103
IMPERIAL_BENCHMARKS_URL := https://github.com/delcypher/fp-benchmarks-imperial.git
IMPERIAL_BENCHMARKS_COMMIT := f2c7c17dd9727233819a7d8598cbffd7ad4a29e0
AACHEN_BENCHMARKS_URL := https://github.com/delcypher/fp-benchmarks-aachen.git
AACHEN_BENCHMARKS_COMMIT := 265b1195e3734dec7c12054bea9919d8d38f0b07

fp_bench_clone:
	git clone  $(FPBENCH_URL) && cd fp-bench && git checkout $(FPBENCH_COMMIT)
	cd fp-bench/benchmarks/c/ && git clone $(IMPERIAL_BENCHMARKS_URL)  imperial && \
		cd imperial && git checkout $(IMPERIAL_BENCHMARKS_COMMIT)
	cd fp-bench/benchmarks/c/ && git clone $(AACHEN_BENCHMARKS_URL) aachen && \
		cd aachen && git checkout $(AACHEN_BENCHMARKS_COMMIT)

fp_bench_build_O2:
	mkdir -p build_O2
	cd build_O2 && \
		CC=wllvm \
		CXX=wllvm++ \
		KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/ \
		KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/Release+Asserts/lib/ \
		LLVM_COMPILER=clang \
		cmake \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DBUILD_IMPERIAL_BENCHMARKS=ON \
		-DBUILD_AACHEN_BENCHMARKS=ON \
		../fp-bench/
	cd build_O2 && LLVM_COMPILER=clang make all create-augmented-spec-file-list -j$(NPROC)
	cd build_O2 && ../fp-bench/svcb/tools/filter-augmented-spec-list.py --categories issta_2017 -- augmented_spec_files.txt > issta_augmented_spec_files.txt
	cd build_O2 && ../fp-bench/svcb/tools/svcb-emit-klee-runner-invocation-info.py issta_augmented_spec_files.txt -o issta_invocation_info.yml

fp_bench_build_O0:
	mkdir -p build_O0
	cd build_O0 && \
		CC=wllvm \
		CXX=wllvm++ \
		KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/ \
		KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/Release+Asserts/lib/ \
		LLVM_COMPILER=clang \
		cmake \
		-DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_IMPERIAL_BENCHMARKS=ON \
		-DBUILD_AACHEN_BENCHMARKS=ON \
		../fp-bench/
	cd build_O0 && LLVM_COMPILER=clang make all create-augmented-spec-file-list -j$(NPROC)
	cd build_O0 && ../fp-bench/svcb/tools/filter-augmented-spec-list.py --categories issta_2017 -- augmented_spec_files.txt > issta_augmented_spec_files.txt
	cd build_O0 && ../fp-bench/svcb/tools/svcb-emit-klee-runner-invocation-info.py issta_augmented_spec_files.txt -o issta_invocation_info.yml


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

.PHONY : all pull gc unshallow clean dist-clean check llvm minisat stp z3 uclibc klee fp_bench_clone fp_bench_build_O0 fp_bench_build_O2 gtest
.DEFAULT_GOAL := all
