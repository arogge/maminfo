LDFLAGS=-lsgutils2
EXECS=mam-info
.PHONY: clean

mam-info: mam-info.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f *.o *~ ./$(EXECS)
