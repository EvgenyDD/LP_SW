set(include_dirs . components laser hal web)

set(srcs 
    "main.c"
    "safety.c"
    "ilda_playground.c"
    "components/hw.c"
    "laser/hl_irq.S"
    "laser/lp.c"
    "web/dns_server.c"
    "web/lan.c"
    "web/web_common.c"
    "web/wifi.c"
    "web/ws.c")

idf_component_register(
    SRCS "${srcs}" 
    INCLUDE_DIRS "${include_dirs}"
    EMBED_FILES web/page/dist/index.html)
