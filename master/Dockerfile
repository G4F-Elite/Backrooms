FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY shared ./shared
COPY master ./master
RUN g++ -std=c++17 -O2 -Wall -Wextra -pedantic -I./shared ./master/src/main.cpp -o /out/backrooms_master

FROM debian:bookworm-slim
WORKDIR /opt/backrooms
COPY --from=build /out/backrooms_master ./backrooms_master
EXPOSE 27017/udp
ENTRYPOINT ["./backrooms_master"]
