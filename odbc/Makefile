CC = cc
CXX = g++
AR = ar
PLATFORM = OS_LINUX
PLATFORM_LDFLAGS = -lodbc -pthread -ldl -lrt -lm
PLATFORM_CFLAGS = -g -D__POSIX__ -Wno-write-strings -fPIC
PLATFORM_INCL = -I. -I..
PLATFORM_LIBDIR = -L/usr/local/lib -L.
PLATFORM_LIBS = -lodbc_wapper
PLATFORM_SHARED_CFLAGS = -fPIC
PLATFORM_SHARED_EXT = so
PLATFORM_SHARED_LDFLAGS = -shared -Wl,-soname -Wl,
PLATFORM_SHARED_VERSIONED = true

.SUFFIXES: .o .cpp

OI_OBJS = Connection.o PreparedStatement.o ResultSet.o ConnPoolMgr.o Exception.o

STATICLIB_NAME := libodbc_wapper.a
BINARY_TEST := test

OI_OBJS_TEST = test.o

all: $(STATICLIB_NAME) $(BINARY_TEST)
	@echo " **********************************************************************"
	@echo " * build successful  [$(STATICLIB_NAME) $(OI_OBJS_TEST)]               "
	@echo " **********************************************************************"
	@echo ""
	@echo ""
	@echo ""
	
.cpp.o:
	$(CXX) $(PLATFORM_CFLAGS) $(PLATFORM_INCL) -c $^ -o $@

$(STATICLIB_NAME): $(OI_OBJS)
	$(AR) -rs $@ $(OI_OBJS)
 
$(BINARY_TEST): $(OI_OBJS_TEST)
	$(CXX) $(PLATFORM_CFLAGS) $(LIB_FLAGS) -o $@ $^ $(PLATFORM_LIBDIR) $(PLATFORM_LDFLAGS) $(PLATFORM_LIBS)

install:
	mkdir -p $(HOME)/include/odbc/
	cp -rf *.h $(HOME)/include/odbc/
	mv $(STATICLIB_NAME) $(HOME)/lib/

clean:
	rm -f *.o $(BINARY_TEST) $(STATICLIB_NAME)
