FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY shared ./shared
COPY server ./server
RUN mkdir -p /out && g++ -std=c++17 -O2 -Wall -Wextra -pedantic -I./shared ./server/src/main.cpp -o /out/backrooms_server

FROM debian:bookworm-slim
WORKDIR /opt/backrooms
COPY --from=build /out/backrooms_server ./backrooms_server
EXPOSE 27015/udp
ENTRYPOINT ["./backrooms_server"]
