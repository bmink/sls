P = sls
OBJS = main.o cgi.o slsobj.o cJSON_helper.o cJSON.o cli.o hiredis_helper.o
CFLAGS = -g -Wall -Wstrict-prototypes
LDLIBS = -lb -lcurl -lhiredis -lxml2 -lcrypto -lreadline
CGI_DIR = /home/ec2-user/devel/bmink.net

$(P): $(OBJS)
	$(CC) -o $(P) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *o; rm -f $(P)

cgi: $(P)
	sudo cp $(P) $(CGI_DIR)/$(P).cgi

