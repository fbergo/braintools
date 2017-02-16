#!/usr/bin/perl

for($i=1;$i<=256;$i++) {
	$nome = "IMG000$i" if ($i > 9);
	$nome = "IMG00$i" if ($i > 99);
	$nome = "IMG0000$i" if ($i < 10);
	$novo = "IMG$i";
	last if (! -f $nome);
	system("mv -f $nome $novo");
}


