#!/usr/bin/perl

# v2

# uso: ecoconvert.pl entrada saida 
# saida sem extensao

if (@ARGV != 2) {
    print "usage: ecoconvert.pl entrada saida\n\nsaida sem extensao\n";
    exit 1;
}

$entrada = shift @ARGV;
$saida   = shift @ARGV;

print ">> dicom2scn\n";
system("dicom2scn -n1 $entrada ${saida}.scn");
print ">> ecodemux\n";
system("ecodemux ${saida}.scn");
unlink("${saida}.scn");

for($i=1;$i<=6;$i++) {
    print "Escrevendo (scn2ana) e${i}_${saida}.hdr + e${i}_${saida}.img\n";
    system("scn2ana -o e${i}_${saida}.hdr e${i}_${saida}.scn >/dev/null");
    unlink("e${i}_${saida}.scn");
}

