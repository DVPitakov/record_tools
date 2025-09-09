#include "postgres.h"

#include "access/htup_details.h"
#include "access/tuptoaster.h"
#include "catalog/pg_type.h"
#include "catalog/namespace.h"
#include "catalog/pg_proc.h"


#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/typcache.h"
#include "utils/fmgroids.h"
#include "utils/snapmgr.h"
#include "utils/rel.h"

#include "nodes/makefuncs.h"

PG_MODULE_MAGIC;

static const char*
rtls_get_attr_as_text_impl(Datum composite, const char* target_attr_name)
{
	HeapTupleHeader td;
	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;
	HeapTupleData tmptup,
			   *tuple;
	int			i;
	const char* result = NULL;

	td = DatumGetHeapTupleHeader(composite);

	/* Extract rowtype info and find a tupdesc */
	tupType = HeapTupleHeaderGetTypeId(td);
	tupTypmod = HeapTupleHeaderGetTypMod(td);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	/* Build a temporary HeapTuple control structure */
	tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
	tmptup.t_data = td;
	tuple = &tmptup;

	for (i = 0; i < tupdesc->natts; i++)
	{
		Datum		val;
		bool		isnull;
		char		*attname;
		Form_pg_attribute att = TupleDescAttr(tupdesc, i);

		if (att->attisdropped)
			continue;

		attname = NameStr(att->attname);

		if(strcmp(attname, target_attr_name) != 0)
			continue;

		val = heap_getattr(tuple, i + 1, tupdesc, &isnull);

		if (!isnull)
		{
			bool typisvarlena;//unused
			Oid outfuncoid;
			getTypeOutputInfo(att->atttypid, &outfuncoid, &typisvarlena);
			result = OidOutputFunctionCall(outfuncoid, val);
		}

		break;
	}

	ReleaseTupleDesc(tupdesc);
	return result;
}


PG_FUNCTION_INFO_V1(rtls_get_attr_as_text);
Datum
rtls_get_attr_as_text(PG_FUNCTION_ARGS)
{
	Datum		record = PG_GETARG_DATUM(0);
	text*		attr_name_txt =  PG_GETARG_TEXT_PP(1);
	const char* attr_name = text_to_cstring(attr_name_txt);

	const char* result = rtls_get_attr_as_text_impl(record, attr_name);
	if(!result)
		PG_RETURN_NULL();

	PG_RETURN_TEXT_P(cstring_to_text(result));
}


PG_FUNCTION_INFO_V1(rtls_get_attr_names);
Datum
rtls_get_attr_names(PG_FUNCTION_ARGS)
{
	typedef struct
	{
		TupleDesc	tupdesc;
		int			attnum;
	} rtls_get_attr_names_fctx;

	FuncCallContext *funcctx;
	rtls_get_attr_names_fctx *fctx;

	if(SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		Datum	record = PG_GETARG_DATUM(0);

		fctx = (rtls_get_attr_names_fctx*) palloc(sizeof(rtls_get_attr_names_fctx));

		HeapTupleHeader td = DatumGetHeapTupleHeader(record);

		Oid		tupType = HeapTupleHeaderGetTypeId(td);
		int32	tupTypmod = HeapTupleHeaderGetTypMod(td);

		fctx->attnum = 0;
		fctx->tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

		funcctx->user_fctx = fctx;
		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();
	fctx = funcctx->user_fctx;

	while (fctx->attnum < fctx->tupdesc->natts)
	{
		Datum		val;
		bool		isnull;
		char		*attname;
		Form_pg_attribute att = TupleDescAttr(fctx->tupdesc, fctx->attnum);

		if (att->attisdropped) {
			fctx->attnum++;
			continue;
		}

		attname = NameStr(att->attname);

		Datum	result;
		result = CStringGetTextDatum(attname);

		fctx->attnum++;
		SRF_RETURN_NEXT(funcctx, result);
	}

	ReleaseTupleDesc(fctx->tupdesc);
	SRF_RETURN_DONE(funcctx);
}


PG_FUNCTION_INFO_V1(rtls_set_attr);
Datum
rtls_set_attr(PG_FUNCTION_ARGS)
{
	Datum		composite = PG_GETARG_DATUM(0);

	text*		attr_name_txt = PG_GETARG_TEXT_PP(1);
	const char* attr_name = text_to_cstring(attr_name_txt);

	text*		attr_value_txt = PG_GETARG_TEXT_PP(2);
	const char* attr_value = text_to_cstring(attr_value_txt);

	Datum		result;

	HeapTupleHeader td;
	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;
	HeapTupleData tmptup,
	           *tuple;
	int			i;

	td = DatumGetHeapTupleHeader(composite);

	/* Extract rowtype info and find a tupdesc */
	tupType = HeapTupleHeaderGetTypeId(td);
	tupTypmod = HeapTupleHeaderGetTypMod(td);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	/* Build a temporary HeapTuple control structure */
	tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
	tmptup.t_data = td;
	tuple = &tmptup;

	Datum		*resultValues;
	bool		*resultNulls;
	HeapTuple	resultTuple;

	resultValues	= (Datum*) palloc(tupdesc->natts * sizeof(Datum));
	resultNulls		= (bool*) palloc(tupdesc->natts * sizeof(bool));

	heap_deform_tuple(tuple, tupdesc, resultValues, resultNulls);

	for (i = 0; i < tupdesc->natts; i++)
	{
		Datum		val;
		bool		isnull;
		char		*attname;
		Form_pg_attribute att = TupleDescAttr(tupdesc, i);

		if (att->attisdropped)
			continue;

		attname = NameStr(att->attname);

		if(strcmp(attname, attr_name) != 0)
			continue;

		Oid inputFuncOid;
		Oid typIOParam;

		FmgrInfo finfo;
		getTypeInputInfo(att->atttypid, &inputFuncOid, &typIOParam);

		fmgr_info(inputFuncOid, &finfo);

		resultValues[i] = InputFunctionCall(&finfo, attr_value, typIOParam, tupTypmod);
		break;
	}


	resultTuple	= heap_form_tuple(tupdesc, resultValues, resultNulls);
	result		= HeapTupleGetDatum(resultTuple);

	ReleaseTupleDesc(tupdesc);
	return result;
}
