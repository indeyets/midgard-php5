--TEST--
test midgard_object_class
--SKIPIF--
<?php if (!extension_loaded("midgard2")) print "skip"; ?>
--INI--
midgard.engine = On
midgard.http = On
midgard.memory_debug = Off
midgard.configuration=
midgard.configuration_file=[[CFG_FILE]]
report_memleaks = Off
--ENV--
MIDGARD_ENV_GLOBAL_SHAREDIR=[[SHARE_PATH]]
--FILE--
<?php
$obj = new atype();
$obj->a = 'a';
$obj->b = 'b';
$obj->create();

$obj2 = new atype();
$obj2->a = 'a';
$obj2->b = 'c';
$obj2->create();

$coll = atype::new_collector('a', 'a');
$coll->set_key_property('a', 'a');
var_dump($coll->execute());

$obj->delete();
$obj2->delete();

?>
===DONE===
--EXPECTF--
bool(true)
===DONE===
