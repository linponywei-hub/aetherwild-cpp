# syntax=docker/dockerfile:1
FROM gcc:14-bookworm AS builder
WORKDIR /src
RUN apt-get update && apt-get install -y --no-install-recommends python3 && rm -rf /var/lib/apt/lists/*
COPY include/ include/
COPY src/ src/
COPY apps/ apps/
COPY tests/ tests/
COPY tools/build.py tools/build.py
COPY tools/install_terrain_textures.py tools/install_terrain_textures.py
COPY web/assets/ web/assets/
RUN python3 tools/install_terrain_textures.py
ENV AW_BUILD_JOBS=1
RUN python3 tools/build.py
RUN python3 tests/http_runner.py && python3 tests/content_http.py && python3 tests/cloud_access.py && python3 tests/bow_http.py build/aetherwild_workstation
RUN mkdir -p /runtime-libs && cp -L "$(g++ -print-file-name=libstdc++.so.6)" /runtime-libs/ && cp -L "$(gcc -print-file-name=libgcc_s.so.1)" /runtime-libs/

FROM debian:bookworm-slim AS runtime
WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends util-linux && rm -rf /var/lib/apt/lists/*
COPY --from=builder /runtime-libs/ /opt/aetherwild/lib/
COPY --from=builder /src/build/aetherwild_workstation /app/aetherwild_workstation
COPY web/ /app/web/
COPY --from=builder /src/web/assets/terrain/ /app/web/assets/terrain/
COPY assets/reference/ /app/assets/reference/
RUN mkdir -p /app/runtime-data && chown -R 10001:10001 /app/runtime-data
ENV LD_LIBRARY_PATH=/opt/aetherwild/lib AETHERWILD_PUBLIC=1 AETHERWILD_DATA_DIR=/app/runtime-data
COPY tools/docker-entrypoint.sh /app/docker-entrypoint.sh
RUN chmod 755 /app/docker-entrypoint.sh
ENTRYPOINT ["/app/docker-entrypoint.sh"]
EXPOSE 10000
CMD ["/app/aetherwild_workstation", "--root", "/app"]
