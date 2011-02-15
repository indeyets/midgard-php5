--TEST--
property-helper test
--SKIPIF--
<?php if (!extension_loaded("midgard2")) print "skip"; ?>
--INI--
midgard.engine = On
midgard.http = On
midgard.memory_debug = Off
midgard.configuration=
midgard.configuration_file=[[CFG_FILE]]
report_memleaks = On
--FILE--
<?php
$obj = new atype();

$meta = $obj->metadata;
$meta2 = $obj->metadata;

var_dump($meta2->creator);

unset($meta);
unset($obj);

var_dump($meta2->creator);
?>
===DONE===
--EXPECTF--
string(0) ""
string(0) ""
===DONE===