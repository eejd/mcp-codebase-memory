# codebase-memory-mcp — container image for the pure-C MCP server binary.
#
# The MCP server speaks JSON-RPC 2.0 over stdio (the upstream/default transport).
# There is NO StreamableHTTP MCP transport in this tree yet; consumers that need
# HTTP must front the stdio binary with a bridge, or wait for a native transport
# (tracked in this fork's issues). The optional HTTP server in src/ui is the
# graph-visualization UI, not an MCP endpoint.
#
# Build mirrors CI (.github/workflows/_build.yml): scripts/build.sh is the single
# source of truth; Ubuntu leg needs only build-essential + zlib1g-dev.
#
# Run (stdio MCP):  docker run -i --rm -v repos:/workspaces:ro ghcr.io/eejd/codebase-memory-mcp

FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential zlib1g-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN scripts/build.sh CC=gcc CXX=g++

FROM debian:bookworm-slim
# git: the binary shells out to `git log` for history parsing when libgit2 is
# not compiled in (the build above omits libgit2 by design — popen fallback).
RUN apt-get update && apt-get install -y --no-install-recommends \
        git ca-certificates zlib1g \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 1000 --create-home cbm
COPY --from=build /src/build/c/codebase-memory-mcp /usr/local/bin/codebase-memory-mcp
USER cbm
WORKDIR /home/cbm
ENTRYPOINT ["codebase-memory-mcp"]
