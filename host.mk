HOST_BUILD_DIR ?= host_build
HOST_AUDIT_DIR ?= out
HOST_AUDIT_FILE ?= $(HOST_AUDIT_DIR)/audit.csv
HOST_SUMMARY_FILE ?= $(HOST_AUDIT_DIR)/summary.txt
HOST_REPLAY ?= data/sample_replay_stable.csv

.PHONY: configure build test sim clean
.PHONY: regression gold

configure:
	cmake -S . -B $(HOST_BUILD_DIR)

build: configure
	cmake --build $(HOST_BUILD_DIR) -j2

test: build
	./$(HOST_BUILD_DIR)/e84_aps_host_tests

sim: build
	mkdir -p $(HOST_AUDIT_DIR)
	./$(HOST_BUILD_DIR)/e84_aps_sim --replay $(HOST_REPLAY) --out $(HOST_AUDIT_FILE) > $(HOST_SUMMARY_FILE)

regression:
	$(MAKE) -f host.mk test
	./scripts/run_fixture_matrix.sh
	./scripts/run_gold_matrix.sh

gold:
	./scripts/run_gold_matrix.sh

clean:
	rm -rf $(HOST_BUILD_DIR) $(HOST_AUDIT_DIR)
