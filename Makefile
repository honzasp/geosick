.PHONY: all local release help docker

.EXPORT_ALL_VARIABLES:

BUILDDIR ?= build
BUILDDIR_RELEASE ?= build.release

.ONESHELL:

all: local

clean:
	rm -rf ${BUILDDIR}
	rm -rf ${BUILDDIR_RELEASE}

local:
	[ -d "${BUILDDIR}" ] || meson ${BUILDDIR} --prefix ${HOME}/.local/
	ninja -C ${BUILDDIR}
	ninja -C ${BUILDDIR} install

release:
	[ -d "${BUILDDIR_RELEASE}"] || meson ${BUILDDIR_RELEASE}
	ninja -C ${BUILDDIR_RELEASE}
	ninja -C ${BUILDDIR_RELEASE} install

docker: Dockerfile
	docker build -f Dockerfile -t geosick .

help:
	@echo "The following targets are available:"
	@echo "===================================="
	@echo
	@echo "  make local        : Build & install into ~/.local prefix"
	@echo "  make release      : Build & install system-wide"
	@echo "  make docker       : Build the docker image" 
	@echo "  make              : Same as 'make local'"
	@echo

