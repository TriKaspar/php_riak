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

#ifndef RIAK_EXCEPTIONS__H__
#define RIAK_EXCEPTIONS__H__

#include <zend_exceptions.h>

extern zend_class_entry *riak_badarguments_exception_ce;

extern zend_class_entry *riak_connection_exception_ce;
extern zend_class_entry *riak_communication_exception_ce;
extern zend_class_entry *riak_response_exception_ce;

extern zend_class_entry *riak_not_found_exception_ce;

void riak_exceptions_init(TSRMLS_D);

#endif
