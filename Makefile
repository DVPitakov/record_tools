MODULES = record_tools

EXTENSION = record_tools
DATA = record_tools--1.0.sql
PGFILEDESC = "record_tools - tools to access data of record"

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
