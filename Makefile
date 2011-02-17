FETCH_CMD=wget
MASTER_SITE=http://www.percona.com/downloads/community
MYSQL_VERSION=5.0.92
PATCHSET=b23
PERCONA_SERVER ?=Percona-Server

all: main autorun
	@echo ""
	@echo "Percona Server source code is ready"
	@echo "Now change directory to $(PERCONA_SERVER) define variables as show below"
	@echo ""
	export CFLAGS="-O2 -g -fmessage-length=0 -D_FORTIFY_SOURCE=2"
	export CXXFLAGS="-O2 -g -fmessage-length=0 -D_FORTIFY_SOURCE=2"
	export LIBS=-lrt
	@echo ""
	@echo "and run ./configure ... --without-plugin-innobase --with-plugin-innodb_plugin && make all install"
	@echo ""

autorun:
	cd $(PERCONA_SERVER) && ./BUILD/autorun.sh

main: mysql-$(MYSQL_VERSION).tar.gz
	@echo "Prepare Percona Server sources"
	rm -rf mysql-$(MYSQL_VERSION)
	rm -rf $(PERCONA_SERVER)
	tar zxf mysql-$(MYSQL_VERSION).tar.gz
	mv mysql-$(MYSQL_VERSION) $(PERCONA_SERVER)
	(cat `cat series`) | patch -p1 -d $(PERCONA_SERVER)

mysql-$(MYSQL_VERSION).tar.gz:
	@echo "Downloading MySQL sources from $(MASTER_SITE)"
	$(FETCH_CMD) $(MASTER_SITE)/mysql-$(MYSQL_VERSION).tar.gz

clean:
	rm -rf mysql-$(MYSQL_VERSION) $(PERCONA_SERVER)
	rm -f mysql-$(MYSQL_VERSION).tar.gz

