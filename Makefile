P = sls
OBJS = main.o cgi.o slsobj.o cJSON_helper.o
CFLAGS = -g -Wall -Wstrict-prototypes
LDLIBS = -lb -lcurl -lhiredis -lxml2 -lcrypto -lreadline

$(P): $(OBJS)
	$(CC) -o $(P) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *o; rm -f $(P)

cgi: $(P)
	sudo cp $(P) /usr/lib/cgi-bin/$(P).cgi

