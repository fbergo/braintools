
Repository of brain imaging tools, mostly developed between 2002 and 2013 at
the Institute of Computing and Neuroimaging Laboratory at Unicamp.

They were originally developed at whatever Linux we used at the labs at the time, and
have been updated to compile on updated and stricter environments (CentOS 6/7, Fedora 20-25).

All software in this repostory is licensed under the GPL v2.
Felipe Bergo, February 2017.

##Directories

- **libip**
 image processing library used by IVS and Aftervoxel
- **aftervoxel**
 3D segmentation/visualization tool, GTK+-based GUI
- **ivs**
 3D semi-automatic segmentation tool,  implements the differential IFT. The algorithms and this software are described in [Falcao2004]. This directory also contains the dicom2scn conversion tool and a few SCN-related command-line utilities (scntool, scn2ana, ana2scn)
- **cppmisc**
 Miscellaneous tools based on the monolithic C++ header I wrote around 2009-2010, including:
 - **splitmri** (command-line, breaks down multi-echo MRI SCNs)
 - **greg** (GUI, registration tool)
 - **braintag** (curvilinear reformatting as per [Bergo2006] and [Bergo2008] and automatic
   brain segmentation as per [Bergo2007])

Most of these tools operate with the SCN 3D image format, which was created internally at Unicamp at
the time as a glorified PGM. A description of the format is available as a man page in ivs/man/scn.5

##External dependencies:
- Perl 5 (for configuration scripts)
- GTK+ 2.x and its development libraries
- libpng, libjpeg, zlib and their development headers (-devel packages in RedHat-ish distros).
- cppmisc dependencies: bzlib, freetype2

## References
- [Falcao2004] A. X. Falcão and F. P. G. Bergo, "Interactive volume segmentation with
  differential image foresting transforms," in IEEE Transactions on Medical Imaging,
  vol. 23, no. 9, pp. 1100-1108, Sept. 2004. https://doi.org/10.1109/TMI.2004.829335

              

