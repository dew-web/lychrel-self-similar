CC ?= gcc
CFLAGS ?= -O2 -w          # 移植性重視（Mac/WSL/Linux）。速くしたいときは make CFLAGS="-O2 -march=native -w"

all: bin/outauto bin/outauto_s bin/coreauto2 bin/coreauto bin/ssgraph bin/hazard bin/lemma_check

# p196 は x86-64 の AVX2 が必要（Apple Silicon では不可）
p196: bin/p196

bin/outauto: src/outauto.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<
bin/outauto_s: src/outauto.c
	mkdir -p bin && $(CC) $(CFLAGS) -DECAP_LOG=21 -o $@ $<
bin/coreauto2: src/coreauto2.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<
bin/coreauto: src/coreauto.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<
bin/ssgraph: src/ssgraph.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<
bin/p196: p196/p196.c
	mkdir -p bin && $(CC) -O3 -march=native -o $@ $<
bin/hazard: p196/hazard.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<
bin/lemma_check: p196/lemma_check.c
	mkdir -p bin && $(CC) $(CFLAGS) -o $@ $<

verify-base9:
	python3 base9/verify_quick.py
	python3 base9/symbolic_proof.py

validate: bin/outauto_s bin/coreauto2
	sh scripts/run_validation.sh

clean:
	rm -rf bin work



# --- 2026-11 追加 ---
env:
	sh scripts/check_env.sh

check: bin/outauto_s bin/coreauto2
	sh scripts/check.sh

carries:
	python3 carries/op.py | tee carries/results/op_output.txt

relax:
	python3 carries/relax_perstep.py --bases 3,4,8,10,12,15,16,17,24,32,64 --jobs 4

half: bin/outauto_s bin/coreauto2
	python3 scripts/halfsearch.py 9 10

.PHONY: all p196 verify-base9 validate clean env check carries relax half
