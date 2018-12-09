CXXFLAGS = -std=c++11
CXX = g++
MPICXX= -I/usr/local/Cellar/open-mpi/3.1.2/include -L/usr/local/opt/libevent/lib -L/usr/local/Cellar/open-mpi/3.1.2/lib -lmpi

OBJS = utils.o
TRGS = error_analysis.exe

all: ${TRGS}

%.exe: %.o ${OBJS}
	${CXX} ${CXXFLAGS} ${MPICXX} ${CPPFLAGS} ${LDFLAGS} $^ ${LDLIBS} -o $@

.SECONDARY:
clean:
	-${RM} *.o *.exe
