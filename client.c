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
#include <zend_exceptions.h>
#include "php_riak.h"
#include "client.h"
#include "object.h"
#include "pool.h"
#include "exceptions.h"

zend_class_entry *riak_client_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_client_ctor, 0, ZEND_RETURN_VALUE, 1)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_client_ping, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry riak_client_methods[] = {
	PHP_ME(RiakClient, __construct, arginfo_client_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(RiakClient, ping, arginfo_client_ping, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void riak_client_init(TSRMLS_D) /* {{{ */
{
	zend_class_entry ce;
 
	INIT_CLASS_ENTRY(ce, "RiakClient", riak_client_methods);
	ce.create_object = create_client_data;
	riak_client_ce = zend_register_internal_class(&ce TSRMLS_CC);
  
	zend_declare_property_null(riak_client_ce, "host", sizeof("host")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(riak_client_ce, "port", sizeof("port")-1, ZEND_ACC_PUBLIC TSRMLS_CC);

	zend_declare_property_null(riak_client_ce, "w", sizeof("w")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(riak_client_ce, "dw", sizeof("dw")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(riak_client_ce, "pw", sizeof("pw")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(riak_client_ce, "r", sizeof("r")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(riak_client_ce, "rw", sizeof("rw")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(riak_client_ce, "pr", sizeof("pr")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
}
/* }}} */

zend_object_value create_client_data(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	client_data *tobj;
 
	tobj = emalloc(sizeof(client_data));
	memset(tobj, 0, sizeof(client_data));

	zend_object_std_init((zend_object *) &tobj->std, class_type TSRMLS_CC);
#if ZEND_MODULE_API_NO >= 20100525
	object_properties_init((zend_object*) &tobj->std, class_type);
#else
	{
		zval *tmp;
		zend_hash_copy(tobj->std.properties, &class_type->default_properties, (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
	}
#endif
	retval.handle = zend_objects_store_put(tobj, (zend_objects_store_dtor_t) zend_objects_destroy_object, 
		(zend_objects_free_object_storage_t) free_client_data, NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();
	return retval;
}
/* }}} */

void free_client_data(void *object TSRMLS_DC) /* {{{ */
{
	client_data* data = (client_data*)object;
	zend_object_std_dtor(&data->std TSRMLS_CC);
	if (data->connection) {
		release_connection(data->connection TSRMLS_CC);
	}

	efree(data);
}
/* }}} */

zval* create_client_object(char* host, long port TSRMLS_DC) /* {{{ */
{
    zval *zclient, *zhost, *zport;

    MAKE_STD_ZVAL(zhost);
    MAKE_STD_ZVAL(zport);
    ZVAL_STRING(zhost, host, 1);
    ZVAL_LONG(zport, port);

	MAKE_STD_ZVAL(zclient);
	object_init_ex(zclient, riak_client_ce);
    CALL_METHOD2(RiakClient, __construct, zclient, zclient, zhost, zport);

    zval_ptr_dtor(&zhost);
    zval_ptr_dtor(&zport);

	return zclient;
}
/* }}} */

/* {{{ proto void RiakClient->__construct(string $host, [int $port])
Create a new RiakClient */
PHP_METHOD(RiakClient, __construct)
{
	int connResult;
	client_data *data;
	char *host, *szHost;
	int hostLen;
	long port = DEFAULT_PORT;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &host, &hostLen, &port) == FAILURE) {
		return;
	}
	zend_update_property_stringl(riak_client_ce, getThis(), "host", sizeof("host")-1, host, hostLen TSRMLS_CC);
	zend_update_property_long(riak_client_ce, getThis(), "port", sizeof("port")-1, port TSRMLS_CC);

	data = (client_data*)zend_object_store_get_object(getThis() TSRMLS_CC);
	data->connection = take_connection(host, hostLen, port TSRMLS_CC);
	if (!data->connection) {
		zend_throw_exception(riak_connection_exception_ce, "Connection error", 1000 TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto void RiakClient->ping()
Ping riak to see if it is alive, an exception is thrown if no response is received */
PHP_METHOD(RiakClient, ping)
{
	int pingStatus;
	riak_connection *connection;

	GET_RIAK_CONNECTION(getThis(), connection);
	ensure_connected(connection TSRMLS_CC);

	pingStatus = riack_ping(connection->client);
	CHECK_RIACK_STATUS_THROW_AND_RETURN_ON_ERROR(connection, pingStatus);
}
/* }}} */
