/*
   Copyright 2013 Trifork A/S
   Author: Kaspar Bach Pedersen

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <php.h>
#include <riack.h>
#include "php_riak.h"
#include "bucket.h"
#include "bucket_properties.h"
#include "connection.h"
#include "object.h"
#include "exceptions.h"
#include "req_inputs.h"
#include "req_outputs.h"

riak_connection *get_riak_connection(zval *zbucket TSRMLS_DC);

zend_class_entry *riak_bucket_ce;

#define RIAK_REQ_PROP_SET_LONG(CLASS_ALIAS, GETTER, TARGET) \
    RIAK_CALL_METHOD(CLASS_ALIAS, GETTER, &ztmp, zconfig); \
    if (Z_TYPE(ztmp) == IS_LONG) { TARGET##_use = 1; TARGET = Z_LVAL(ztmp); }

#define RIAK_REQ_PROP_SET_BOOL(CLASS_ALIAS, GETTER, TARGET) \
    RIAK_CALL_METHOD(CLASS_ALIAS, GETTER, &ztmp, zconfig); \
    if (Z_TYPE(ztmp) == IS_BOOL && Z_BVAL(ztmp)) { TARGET##_use = TARGET = 1; }

#define GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR( VAR ) VAR = get_riak_connection(getThis() TSRMLS_CC); \
  if (!VAR) { \
      zend_throw_exception(riak_badarguments_exception_ce, "No client", 500 TSRMLS_CC); \
      return; \
  } 

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_ctor, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, client)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_put, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, object)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_get, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_delete, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_fetchprops, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_applyprops, 0, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, bucket_properties)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_indexq, 0, ZEND_RETURN_VALUE, 2)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, from)
    ZEND_ARG_INFO(0, to)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_streamkeys, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, keystreamer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bucket_listkeys, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry riak_bucket_methods[] = {
	PHP_ME(RiakBucket, __construct, arginfo_bucket_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(RiakBucket, put, arginfo_bucket_put, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucket, get, arginfo_bucket_get, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucket, delete, arginfo_bucket_delete, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucket, indexQuery, arginfo_bucket_indexq, ZEND_ACC_PUBLIC)

	PHP_ME(RiakBucket, fetchProperties, arginfo_bucket_fetchprops, ZEND_ACC_PUBLIC)
	PHP_ME(RiakBucket, applyProperties, arginfo_bucket_applyprops, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucket, streamKeys, arginfo_bucket_streamkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RiakBucket, listKeys, arginfo_bucket_listkeys, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void riak_bucket_init(TSRMLS_D) /* {{{ */
{
	zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "RiakBucket", riak_bucket_methods);
	riak_bucket_ce = zend_register_internal_class(&ce TSRMLS_CC);

    zend_declare_property_null(riak_bucket_ce, "name", sizeof("name")-1, ZEND_ACC_PROTECTED TSRMLS_CC);
    zend_declare_property_null(riak_bucket_ce, "client", sizeof("client")-1, ZEND_ACC_PROTECTED TSRMLS_CC);
}
/* }}} */

zval* create_bucket_object(zval* zclient, char* name, int name_len TSRMLS_DC) /* {{{ */
{
	zval *zbucket, *zname;
	MAKE_STD_ZVAL(zbucket);
	MAKE_STD_ZVAL(zname);

    ZVAL_STRINGL(zname, name, name_len, 1);

	object_init_ex(zbucket, riak_bucket_ce);
	RIAK_CALL_METHOD2(RiakBucket, __construct, zbucket, zbucket, zclient, zname);

	zval_ptr_dtor(&zname);
	return zbucket;
}
/* }}} */

/* {{{ proto void RiakBucket->__construct(RiakConnection $client, string $name)
Create a new RiakBucket */
PHP_METHOD(RiakBucket, __construct)
{
	char *name;
	int nameLen;
    zval* client, *zprop;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "os", &client, &name, &nameLen) == FAILURE) {
		return;
	}
	zend_update_property_stringl(riak_bucket_ce, getThis(), "name", sizeof("name")-1, name, nameLen TSRMLS_CC);
	zend_update_property(riak_bucket_ce, getThis(), "client", sizeof("client")-1, client TSRMLS_CC);
}
/* }}} */

struct riak_stream_key_cb_param {/* {{{ */
#ifdef ZTS
    TSRMLS_D;
#endif
    zval *zstreamer;
};
/* }}} */

void riak_stream_key_cb(struct RIACK_CLIENT* c, void* p, RIACK_STRING key) {/* {{{ */
    zval *zkey, zret, zfuncname;

    struct riak_stream_key_cb_param *param = (struct riak_stream_key_cb_param*)p;
    MAKE_STD_ZVAL(zkey);
    ZVAL_STRINGL(zkey, key.value, key.len, 1);
    ZVAL_STRING(&zfuncname, "process", 0);
#ifdef ZTS
    call_user_function(NULL, &param->zstreamer, &zfuncname, &zret, 1, &zkey, param->tsrm_ls);
#else
    call_user_function(NULL, &param->zstreamer, &zfuncname, &zret, 1, &zkey);
#endif

    zval_ptr_dtor(&zkey);
}
/* }}} */

/* {{{ proto void RiakBucket->streamKeys(RiakKeyStreamer streamer)
Streams all keys in the bucket */
PHP_METHOD(RiakBucket, streamKeys)
{
    struct riak_stream_key_cb_param cb_params;
    RIACK_STRING rsbucket;
    riak_connection *connection;
    zval* zstreamer;
    int riackstatus;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zstreamer) == FAILURE) {
        return;
    }
    GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)
    rsbucket = riack_name_from_bucket(getThis() TSRMLS_CC);
#ifdef ZTS
    cb_params.tsrm_ls = TSRMLS_C;
#endif
    cb_params.zstreamer = zstreamer;
    riackstatus = riack_stream_keys(connection->client, rsbucket, riak_stream_key_cb, &cb_params);
    CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackstatus);
}
/* }}} */


/* {{{ proto string[] RiakBucket->listKeys()
List all keys in the bucket */
PHP_METHOD(RiakBucket, listKeys)
{
    struct RIACK_STRING_LINKED_LIST* resultlist, *curr;
    RIACK_STRING rsbucket;
    riak_connection *connection;
    zval* zresultarr;
    int riackstatus;
    GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)
    rsbucket = riack_name_from_bucket(getThis() TSRMLS_CC);
    riackstatus = riack_list_keys(connection->client, rsbucket, &resultlist);
    CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackstatus);

    MAKE_STD_ZVAL(zresultarr);
    array_init(zresultarr);
    curr = resultlist;
    while (curr) {
        add_next_index_stringl(zresultarr, curr->string.value, curr->string.len, 1);
        curr = curr->next;
    }
    riack_free_string_linked_list(connection->client, &resultlist);

    RETURN_ZVAL(zresultarr, 0, 1);
}
/* }}} */


/* {{{ proto array RiakBucket->indexQuery(string $index, string $from [, string $to])
Apply given properties to this bucket */
PHP_METHOD(RiakBucket, indexQuery)
{
    RIACK_STRING rsbucket, rsindex, rsfrom, rsto;
    RIACK_STRING_LIST resultlist;
    char *index, *from, *to;
    int indexlen, fromlen, tolen, riackstatus, i;
    riak_connection *connection;
    zval *zresult;
    tolen = 0; to = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|s", &index, &indexlen, &from, &fromlen, &to, &tolen) == FAILURE) {
        return;
    }

    GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)
    rsbucket = riack_name_from_bucket(getThis() TSRMLS_CC);
    rsindex.len = indexlen;
    rsindex.value = index;
    rsfrom.len = fromlen;
    rsfrom.value = from;
    if (to != NULL && tolen > 0) {
        rsto.len = tolen;
        rsto.value = to;
        riackstatus = riack_2i_query_range(connection->client, rsbucket, rsindex, rsfrom, rsto, &resultlist);
    } else {
        riackstatus = riack_2i_query_exact(connection->client, rsbucket, rsindex, rsfrom, &resultlist);
    }
    CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackstatus);

    MAKE_STD_ZVAL(zresult);
    array_init(zresult);
    for (i=0; i<resultlist.string_count; ++i) {
        add_next_index_stringl(zresult, resultlist.strings[i].value, resultlist.strings[i].len, 1);
    }
    riack_free_string_list(connection->client, &resultlist);
    RETURN_ZVAL(zresult, 0, 1);
}
/* }}} */

/* {{{ proto void RiakBucket->applyProperties(RiakBucketProperties $properties)
Apply given properties to this bucket */
PHP_METHOD(RiakBucket, applyProperties)
{
	riak_connection *connection;

	RIACK_STRING bucketName;
    zval* zpropsObj, znval, zallowmult;
	int riackResult;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &zpropsObj) == FAILURE) {
		return;
	}
	GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)
    bucketName = riack_name_from_bucket(getThis() TSRMLS_CC);

    RIAK_CALL_METHOD(RiakBucketProperties, getNValue, &znval, zpropsObj);
    RIAK_CALL_METHOD(RiakBucketProperties, getAllowMult, &zallowmult, zpropsObj);

    riackResult = riack_set_bucket_props(connection->client, bucketName, Z_LVAL(znval), Z_BVAL(zallowmult));
 	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackResult);
}
/* }}} */

/* {{{ proto RiakBucketProperties RiakBucket->fetchProperties()
Fetch and return a RiakBucketProperties object with properties for this bucket */
PHP_METHOD(RiakBucket, fetchProperties)
{
	riak_connection *connection;
	RIACK_STRING bucketName;
	uint32_t nVal = 3;
	uint8_t allowMult = 0;
	int riackResult;
	zval *zBucketProps, *zAllowMult, *zNVal;

	GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)

    bucketName = riack_name_from_bucket(getThis() TSRMLS_CC);
	riackResult = riack_get_bucket_props(connection->client,  bucketName, &nVal, &allowMult);

	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackResult);

	MAKE_STD_ZVAL(zNVal);
	ZVAL_LONG(zNVal, nVal);

	MAKE_STD_ZVAL(zAllowMult);
	ZVAL_BOOL(zAllowMult, allowMult);

	MAKE_STD_ZVAL(zBucketProps);
	object_init_ex(zBucketProps, riak_bucket_properties_ce);
	RIAK_CALL_METHOD2(RiakBucketProperties, __construct, zBucketProps, zBucketProps, zNVal, zAllowMult);
	RETVAL_ZVAL(zBucketProps, 0, 1);

	zval_ptr_dtor(&zNVal);
	zval_ptr_dtor(&zAllowMult);
}
/* }}} */

/* {{{ proto void RiakBucket->delete(RiakObject|string $object)
Deletes given object from riak */
PHP_METHOD(RiakBucket, delete)
{
	struct RIACK_DEL_PROPERTIES props;
	riak_connection *connection;
    zval *zparam, *zTmp, *zconfig;
	RIACK_STRING bucketName, key;
	int riackResult;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|o", &zparam, &zconfig) == FAILURE) {
		return;
	}
    memset(&props, 0, sizeof(props));
	GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)

    /* Set bucket name */
    bucketName = riack_name_from_bucket(getThis() TSRMLS_CC);
    if (Z_TYPE_P(zparam) == IS_STRING) {
        key.len = Z_STRLEN_P(zparam);
        key.value = Z_STRVAL_P(zparam);
    } else {
        /* If zparam is not a string it should be a RiakObject, get the key from the object. */
        GET_PROPERTY_INTO_RIACK_STR_OR_ELSE(riak_object_ce, zparam, "key", zTmp, key) {
            zend_throw_exception(riak_badarguments_exception_ce, "key missing from object", 5001 TSRMLS_CC);
            return;
        }
    }

    if (zconfig != NULL && Z_TYPE_P(zconfig) == IS_OBJECT) {
        zval ztmp;
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getR, props.r);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getPR, props.pr);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getRW, props.rw);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getW, props.w);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getDW, props.dw);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_DeleteInput, getPW, props.pw);
        RIAK_CALL_METHOD(Riak_Input_DeleteInput, getVClock, &ztmp, zconfig);
        if (Z_TYPE(ztmp) == IS_STRING) {
            props.vclock.len = Z_STRLEN(ztmp);
            props.vclock.clock = (uint8_t*)Z_STRVAL(ztmp);
        }
    }

    /* vclock ? */
	riackResult = riack_delete(connection->client, bucketName, key, &props);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackResult);
}
/* }}} */

/* {{{ proto Riak\Object RiakBucket->put(Riak\Object $object [, RiakPutRequestConfiguration $configuration])
Store a RiakObject in riak, if something goes wrong an RiakException is thrown */
PHP_METHOD(RiakBucket, put)
{
    int riackResult;
    zval *zObject, *zTmp, *zconfig;
	struct RIACK_OBJECT obj, returnedObj;
	struct RIACK_CONTENT riackContent;
	struct RIACK_PUT_PROPERTIES props;
	riak_connection *connection;
    long options;

    options = 0;
    zconfig = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o|o", &zObject, &zconfig) == FAILURE) {
		return;
	}
	GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)

	memset(&obj, 0, sizeof(obj));
	memset(&returnedObj, 0, sizeof(returnedObj));
	memset(&riackContent, 0, sizeof(riackContent));
    memset(&props, 0, sizeof(props));
    /* fill content */
    set_riak_content_from_object(&riackContent, zObject, connection->client TSRMLS_CC);

    if (zconfig != NULL && Z_TYPE_P(zconfig) == IS_OBJECT) {
        zval ztmp;
        RIAK_REQ_PROP_SET_BOOL(Riak_Input_PutInput, getReturnHead, props.return_head);
        RIAK_REQ_PROP_SET_BOOL(Riak_Input_PutInput, getReturnBody, props.return_body);
        RIAK_REQ_PROP_SET_BOOL(Riak_Input_PutInput, getIfNotModified, props.if_not_modified);
        RIAK_REQ_PROP_SET_BOOL(Riak_Input_PutInput, getIfNoneMatch, props.if_none_match);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_PutInput, getW, props.w);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_PutInput, getDW, props.dw);
        RIAK_REQ_PROP_SET_LONG(Riak_Input_PutInput, getPW, props.pw);
        RIAK_CALL_METHOD(Riak_Input_PutInput, getVClock, &ztmp, zconfig);
        if (Z_TYPE(ztmp) == IS_STRING) {
            obj.vclock.len = Z_STRLEN(ztmp);
            obj.vclock.clock = (uint8_t*)Z_STRVAL(ztmp);
        }
    }
    /* Set bucket name */
    obj.bucket = riack_name_from_bucket(getThis() TSRMLS_CC);
	obj.content_count = 1;
	obj.content = &riackContent;

    /* No ket provided on function call, get it from RiakObject */
    GET_PROPERTY_INTO_RIACK_STR_OR_ELSE(riak_object_ce, zObject, "key", zTmp, obj.key) {
        // Key is null this is ok
        obj.key.len = 0;
        obj.key.value = 0;
    }
	riackResult = riack_put(connection->client, obj, &returnedObj, &props);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackResult);

    if (returnedObj.vclock.len > 0) {
        MAKE_STD_ZVAL(zTmp);
        ZVAL_STRINGL(zTmp, (char*)returnedObj.vclock.clock, returnedObj.vclock.len, 1);
        zend_update_property(riak_object_ce, zObject, "vclock", sizeof("vclock")-1, zTmp TSRMLS_CC);
        zval_ptr_dtor(&zTmp);
    }
    if (returnedObj.key.len > 0) {
        MAKE_STD_ZVAL(zTmp);
        ZVAL_STRINGL(zTmp, (char*)returnedObj.key.value, returnedObj.key.len, 1);
        zend_update_property(riak_object_ce, zObject, "key", sizeof("key")-1, zTmp TSRMLS_CC);
        zval_ptr_dtor(&zTmp);
    }
    riack_free_object(connection->client, &returnedObj);
    RETURN_ZVAL(zObject, 1, 0);
}
/* }}} */

/* {{{ proto Output\GetOutput RiakBucket->get(string $key [, Riak\Input\GetInput $input])
Get value from riak */
PHP_METHOD(RiakBucket, get)
{
	char *key;
    int keyLen, riackResult;
	size_t contentCount;
    zval *zKey, *zinput;
	struct RIACK_GET_PROPERTIES props;
	struct RIACK_GET_OBJECT getResult;
	RIACK_STRING rsBucket, rsKey;
	riak_connection *connection;
	
    zinput = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|o", &key, &keyLen, &zinput) == FAILURE) {
		return;
	}
	
	GET_RIAK_CONNECTION_RETURN_EXC_ON_ERROR(connection)

	MAKE_STD_ZVAL(zKey);
	ZVAL_STRINGL(zKey, key, keyLen, 1);

	memset(&props, 0, sizeof(props));
	memset(&getResult, 0, sizeof(getResult));

    /* TODO Move this to its own function */
    if (zinput != NULL && Z_TYPE_P(zinput) == IS_OBJECT) {
        zval ztmp;
        RIAK_CALL_METHOD(Riak_Input_GetInput, getReturnHead, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_BOOL && Z_BVAL(ztmp)) {
            props.head_use = props.head = 1;
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getR, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_LONG) {
            props.r_use = 1;
            props.r = Z_LVAL(ztmp);
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getPR, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_LONG) {
            props.pr_use = 1;
            props.pr = Z_LVAL(ztmp);
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getBasicQuorum, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_BOOL && Z_BVAL(ztmp)) {
            props.basic_quorum_present = props.basic_quorum = 1;
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getNotFoundOk, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_BOOL && Z_BVAL(ztmp)) {
            props.notfound_ok_present = props.notfound_ok = 1;
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getReturnDeletedVClock, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_BOOL && Z_BVAL(ztmp)) {
            props.deletedvclock_present = props.deletedvclock = 1;
        }
        RIAK_CALL_METHOD(Riak_Input_GetInput, getIfModifiedVClock, &ztmp, zinput);
        if (Z_TYPE(ztmp) == IS_STRING) {
            props.if_modified_use = 1;
            props.if_modified.len = Z_STRLEN(ztmp);
            props.if_modified.clock = (uint8_t*)Z_STRVAL(ztmp);
        }
    }

    rsBucket = riack_name_from_bucket(getThis() TSRMLS_CC);
	rsKey.len = keyLen;
	rsKey.value = key;
	riackResult = riack_get(connection->client, rsBucket, rsKey, &props, &getResult);
    if (riackResult == RIACK_SUCCESS) {
        contentCount = getResult.object.content_count;
        if (contentCount > 0) {
            zval *zout = get_output_from_riack_get_object(&getResult, zKey TSRMLS_CC);
            RETVAL_ZVAL(zout, 0, 1);
		} else {
            /* Throw not found exception */
            zend_throw_exception(riak_not_found_exception_ce, "Not Found", 2000 TSRMLS_CC);
        }
		riack_free_get_object(connection->client, &getResult);
	} else {
		zval_ptr_dtor(&zKey);
		CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, riackResult);
	}
    zval_ptr_dtor(&zKey);
}
/* }}} */

zval* object_from_riak_content(zval* key, struct RIACK_CONTENT* content TSRMLS_DC)/* {{{ */
{
	zval *object;
	MAKE_STD_ZVAL(object);
	object_init_ex(object, riak_object_ce);
	RIAK_CALL_METHOD1(RiakObject, __construct, object, object, key);

	set_object_from_riak_content(object, content TSRMLS_CC);

	return object;
}
/* }}} */

void riak_name_from_bucket(zval* bucket, char **name, int *namelen TSRMLS_DC)/* {{{ */
{
    zval *zname = zend_read_property(riak_bucket_ce, bucket, "name", sizeof("name")-1, 1 TSRMLS_CC);
    if (Z_TYPE_P(zname) == IS_STRING) {
        *name = Z_STRVAL_P(zname);
        *namelen = Z_STRLEN_P(zname);
    } else {
        *name = NULL;
        *namelen = 0;
    }
}
/* }}} */

RIACK_STRING riack_name_from_bucket(zval* bucket TSRMLS_DC)/* {{{ */
{
    RIACK_STRING bucketName;
    int namelen;
    riak_name_from_bucket(bucket, &bucketName.value, &namelen TSRMLS_CC);
    bucketName.len = namelen;
    return bucketName;
}
/* }}} */

riak_connection *get_riak_connection(zval *zbucket TSRMLS_DC)/* {{{ */
{
    zval *zclient;
    riak_connection *connection = NULL;
    zclient = zend_read_property(riak_bucket_ce, zbucket, "client", sizeof("client")-1, 1 TSRMLS_CC);
    if (zclient) {
        GET_RIAK_CONNECTION(zclient, connection);
		ensure_connected(connection TSRMLS_CC);
    }
	return connection;
}
/* }}} */

