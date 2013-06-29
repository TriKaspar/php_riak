--TEST--
Test that client auto reconnect if timeout passed
--INI--
riak.persistent.timeout=1
--FILE--
<?php
include_once "connect.inc";
$c = new \Riak\Connection($host, $port);
$c->ping();
sleep(3);
$c->ping();
echo RiakStats::getNumReconnects().PHP_EOL;
?>
--EXPECT--
1
