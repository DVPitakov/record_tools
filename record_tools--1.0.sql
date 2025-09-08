CREATE OR REPLACE FUNCTION rtls_get_attr_as_text(record, text)
    RETURNS text AS '$libdir/record_tools.so', 'rtls_get_attr_as_text'
    LANGUAGE c IMMUTABLE STRICT SECURITY INVOKER
    COST 1;

CREATE OR REPLACE FUNCTION rtls_get_attr_names(record)
    RETURNS SETOF text AS '$libdir/record_tools.so', 'rtls_get_attr_names'
    LANGUAGE c IMMUTABLE STRICT SECURITY INVOKER
    COST 1;
