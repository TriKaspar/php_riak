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

#ifndef RIAK_BUCKET__H__
#define RIAK_BUCKET__H__

#include <php.h>
#include <riack.h>

extern zend_class_entry *riak_bucket_ce;

zval* create_bucket_object(zval* zclient, char* name, int name_len TSRMLS_DC);

void riak_bucket_init(TSRMLS_D);

PHP_METHOD(RiakBucket, __construct);
PHP_METHOD(RiakBucket, applyProperties);
PHP_METHOD(RiakBucket, fetchProperties);
PHP_METHOD(RiakBucket, put);
PHP_METHOD(RiakBucket, get);
PHP_METHOD(RiakBucket, delete);
PHP_METHOD(RiakBucket, indexQuery);

PHP_METHOD(RiakBucket, streamKeys);
PHP_METHOD(RiakBucket, listKeys);

zval* object_from_riak_content(zval* key, struct RIACK_CONTENT* content TSRMLS_DC);
RIACK_STRING riack_name_from_bucket(zval* bucket TSRMLS_DC);
void riak_name_from_bucket(zval* bucket, char **name, int *namelen TSRMLS_DC);

#endif
