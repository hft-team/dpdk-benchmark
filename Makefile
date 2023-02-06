TOPDIR=..

ifeq ($(FF_PATH),)
	FF_PATH=${TOPDIR}
endif

ifneq ($(shell pkg-config --exists libdpdk && echo 0),0)
$(error "No installation of DPDK found, maybe you should export environment variable `PKG_CONFIG_PATH`")
endif

PKGCONF ?= pkg-config

CFLAGS += -O -gdwarf-2 $(shell $(PKGCONF) --cflags libdpdk)

LIBS+= $(shell $(PKGCONF) --static --libs libdpdk)
LIBS+= -L${FF_PATH}/lib -Wl,--whole-archive,-lfstack,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

TARGET="helloworld"
all:
	cc ${CFLAGS} -DINET6 -o ${TARGET} main.c ${LIBS}
	cc ${CFLAGS} -o ${TARGET}_epoll main_epoll.c ${LIBS}
	cc ${CFLAGS} -o ping_fstack ping_fstack.c ${LIBS}
	cc ${CFLAGS} -o ping_fstack_epoll ping_fstack_epoll.c ${LIBS}
	cc -o ping_epoll ping_epoll.c -lpthread
	cc -o ping ping.c
.PHONY: clean
clean:
	rm -f *.o ${TARGET} ${TARGET}_epoll
	rm -f ping_fstack

run_ping_fstack:
	./ping_fstack --conf config.ini  --proc-type=primary --proc-id=0