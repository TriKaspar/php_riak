/*
   Copyright 2013: Kaspar Bach Pedersen

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

#ifndef RIAK_OUTPUT__INDEX_OUTPUT__H__
#define RIAK_OUTPUT__INDEX_OUTPUT__H__

#include "php_riak_internal.h"

extern zend_class_entry *riak_index_output_ce;

void riak_output_index_output_init(TSRMLS_D);

// zval *riak_index_output_from_string_list_and_continuation(RIACK_STRING_LIST *result_keys, RIACK_STRING *continuation TSRMLS_DC);

PHP_METHOD(Riak_Index_Output, __construct);
PHP_METHOD(Riak_Index_Output, getResultList);
PHP_METHOD(Riak_Index_Output, getContinuation);

#endif //RIAK_OUTPUT__INDEX_OUTPUT__H__
