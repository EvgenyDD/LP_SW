set(include_dirs . comm laser web ../../common)

set(srcs 
    "error.c"
    "ilda_playground.c"
    "main.c"
    "proto_l1.c"
    "safety.c"
    "comm/cobs.c"
    "comm/crc16_ccitt.c"
    "comm/proto_l0.c"
    "comm/serial.c"
    "laser/hl_irq.S"
    "laser/lp.c"
    "web/dns_server.c"
    "web/lan.c"
    "web/lc_udp.c"
    "web/web_common.c"
    "web/wifi.c"
    "web/ws.c")

idf_component_register(
    SRCS "${srcs}" 
    INCLUDE_DIRS "${include_dirs}"
    EMBED_FILES web/page/dist/index.html)

target_link_libraries(${COMPONENT_TARGET} "-u ld_include_xt_highint5")