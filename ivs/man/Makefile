

MAN1 = ana2scn.1 dicom2scn.1 ivs.1 scn2ana.1 scntool.1
MAN5 = mvf.5 scn.5

ALLMAN = $(MAN1) $(MAN5)

psman:
	groff -t -e -mandoc -rC1 -Tps $(ALLMAN) > manpages.ps

clean:
	rm -f manpages.ps *~

