# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'
#########################

# change 'tests => 1' to 'tests => last_test_to_print';

sub result {
    my($i, $state) = @_;
    if ($state) {
	print "ok $i\n";
    } else {
	print "ng $i\n";
	exit $i;
    }
}

sub sortFunc {
    my($a, $b) = @_;
    my($va, $vb);

    $va = $a->objValue();
    $vb = $b->objValue();
    return $va <=> $vb;
}

use Test;
BEGIN { plan tests => 13 };
use ClearSilver;
ok(1); # If we made it this far, we're ok.

$testnum = 2;
#
# test new()
#
$hdf = ClearSilver::HDF->new();
$hdf ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# test reading file
#
$ret = $hdf->readFile("test.hdf");
$ret ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# test getObj() 
#
$lev2_node = $hdf->getObj("TopNode.2nd1");
$lev2_node ? result($testnum, 1) : result($testnum, 0);
$testnum++;


#
# test objName()
#
$lev2_name = $lev2_node->objName();
($lev2_name eq "2nd1") ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# test objChild() & objValue()
# 
$lev3_node = $lev2_node->objChild();
if (!$lev3_node) {
    result($testnum, 0);
}
$val = $lev3_node->objValue();
($val eq "value1") ?  result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# test objNext()
#
$next_node = $lev3_node->objNext();
if (!$lev3_node) {
    result($testnum, 0);
}
$nam = $next_node->objName();
($nam eq "Entry2") ? result($testnum, 1) : result($testnum, 0); 
$testnum++;

#
# test getChild()
#
$lev2_node = $hdf->getChild("TopNode.2nd1");
$lev2_node ? result($testnum, 1) : result($testnum, 0); 
$testnum++;

#
# test setValue() & getValue()
#
$hdf->setValue("Data.1", "Value1");
$str = $hdf->getValue("Data.1", "default");
($str eq "Value1") ? result($testnum, 1) : result($testnum, 0); 
$testnum++;

$str = $hdf->getValue("Data.2", "default"); # doesn't exist
($str eq "default") ? result($testnum, 1) : result($testnum, 0);     
$testnum++;

#
# test copy tree
# 
$copy = ClearSilver::HDF->new();
$ret = $copy->copy("", $hdf);
$ret ? result($testnum, 0) : result($testnum, 1);
$testnum++;
$str = $copy->getValue("Data.1", "default");
print $str
($str eq "Value1") ? result($testnum, 1) : result($testnum, 0);     
$testnum++;
  
#
# test setSymlink()
#
$ret = $copy->setSymlink( "BottomNode" ,"TopNode");
$ret ? result($testnum, 1) : result($testnum, 0);
$testnum++;
$tmp = $copy->getObj("BottomNode.2nd1");
$tmp ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# test removeTree()
#
$ret = $copy->removeTree("TopNode");
$ret ? result($testnum, 1) : result($testnum, 0);
$testnum++;
$tmp = $copy->getObj("TopNode.2nd1");
$tmp ? result($testnum, 0) : result($testnum, 1);
$testnum++;

#
# test sortObj()
#
$sort_top = $hdf->getObj("Sort.Data");
$sort_top->sortObj("sortFunc");
$child = $sort_top->objChild();
$name = $child->objName();
($name eq "entry3") ? result($testnum, 1) : result($testnum, 0); 
$testnum++;


#
# test CS
#
$cs = ClearSilver::CS->new($hdf);
$cs ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# parseString() and render()
#
$ret = $cs->parseString("<?cs var:TopNode.2nd1.Entry3 ?>");
if (!$ret) {
    result($testnum, 0);
}
$ret = $cs->render();
($ret eq "value3") ? result($testnum, 1) : result($testnum, 0);
$testnum++;

#
# parseFile()
#
$ret = $cs->parseFile("test.cs");
if (!$ret) {
    result($testnum, 0);
}
$ret = $cs->render();
open(FH, "> test.out");
print FH $ret;
close(FH);
$ret = system("diff test.gold test.out > /dev/null");
$ret ? result($testnum, 0) : result($testnum, 1); 


