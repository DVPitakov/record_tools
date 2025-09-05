CREATE OR REPLACE FUNCTION rtls_get_attr_as_text(record, text)
    RETURNS text AS '$libdir/record_tools.so', 'rtls_get_attr_as_text'
    LANGUAGE c IMMUTABLE STRICT SECURITY INVOKER
    COST 1;
