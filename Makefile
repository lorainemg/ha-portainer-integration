.PHONY: help build test run shell clean image

COMPOSE := docker compose run --rm ha-portainer

help:  ## Show available targets
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  make %-8s %s\n", $$1, $$2}'

build:  ## Configure and compile
	$(COMPOSE) bash -c "cmake -B build && cmake --build build --parallel"

test:  ## Configure, compile, and run all tests
	$(COMPOSE) bash -c "cmake -B build && cmake --build build --parallel && ctest --test-dir build --output-on-failure"

run:  ## Configure, compile, and run the binary
	$(COMPOSE) bash -c "cmake -B build && cmake --build build --parallel && ./build/ha-portainer"

shell:  ## Drop into a bash inside the container
	$(COMPOSE) bash

clean:  ## Remove the build/ directory
	rm -rf build

image:  ## Rebuild the Docker image (only needed when Dockerfile changes)
	docker compose build