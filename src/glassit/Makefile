include ../../config.mk

OBJ = $(patsubst %.c, %.lo, $(wildcard *.c))

all: $(GITARGET)

clean:
	$(RM) $(GITARGET) $(OBJ)

$(GITARGET): $(OBJ)
	@echo "$(CC) $@"
	$(Q)$(CC) $(OBJ) $(GILDFLAGS) -o $@

%.lo: %.c
	@echo "${CC} $@"
	$(Q)$(CC) $(GICPPFLAGS) $(GICFLAGS) -fPIC -c -o $@ $<

-include $(OBJ:.lo=.d)

.PHONY: all clean
