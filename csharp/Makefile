
# default values....

CSHARP_CC=$(CSHARP_PATH)/bin/mcs
MONO_JIT=$(CSHARP_PATH)/bin/mono

# common build environment

ifeq ($(NEOTONIC_ROOT),)
NEOTONIC_ROOT = ..
endif

include $(NEOTONIC_ROOT)/rules.mk

# our targets

TARGETS = clearsilver.dll cstest.exe csperftest.exe testcs

all: $(TARGETS)

clearsilver.dll: CS.cs
	$(CSHARP_CC) -target:library -unsafe CS.cs -out:clearsilver.dll

cstest.exe: clearsilver.dll ../dso/libneo.so cstest.cs
	$(CSHARP_CC) -r:clearsilver.dll -unsafe cstest.cs 

csperftest.exe: clearsilver.dll ../dso/libneo.so csperftest.cs
	$(CSHARP_CC) -r:clearsilver.dll -unsafe csperftest.cs

perf: csperftest.exe
	export LD_LIBRARY_PATH=../dso; \
	$(MONO_JIT) csperftest.exe 
	

testcs: cstest.exe
	@echo "Running csharp test"
	@failed=0; \
	rm -f cstest.out; \
	export LD_LIBRARY_PATH=../dso; \
	$(MONO_JIT) cstest.exe > cstest.out; \
	diff cstest.out cstest.gold > /dev/null; \
	return_code=$$?; \
	if [ $$return_code -ne 0 ]; then \
	  diff cstest.out cstest.gold > cstest.err; \
	  echo "Failed csharp test: cstest.cs"; \
	  echo "    See cstest.out and cstest.err"; \
	  failed=1; \
	fi; \
	if [ $$failed -eq 1 ]; then \
	  exit 1; \
	fi;
	@echo "Passed csharp test"

gold: cstest.exe
	export LD_LIBRARY_PATH=../dso; \
	$(MONO_JIT) cstest.exe > cstest.gold;
	@echo "Generated gold files"


clean:
	rm -f core.*

distclean:
	rm -f $(TARGETS) core.* Makefile.depend
