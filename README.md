
Repository of brain imaging tools, mostly developed between 2002 and 2013 at
the Institute of Computing and at the Neuroimaging Laboratory at Unicamp.

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
- [Bergo2006] F. P. G. Bergo and A. X. Falcão, "Fast and automatic curvilinear reformatting of MR images
  of the brain for diagnosis of dysplastic lesions," 3rd IEEE International Symposium on Biomedical
  Imaging: Nano to Macro, 2006., Arlington, VA, 2006, pp. 486-489. https://doi.org/10.1109/ISBI.2006.1624959
- [Bergo2007] F. P. G. Bergo, A. X. Falcão, P. A. V. Miranda and A. X. Falcão, "Automatic Image Segmentation
  by Tree Pruning", J. Math. Imag. Vision, vol. 29, no. 2, Nov. 2007, pp.141-162. https://doi.org/10.1007/s10851-007-0035-4
- [Bergo2008] F. P. G. Bergo, A. X. Falcão, C. L. Yasuda and F. Cendes, "FCD segmentation using texture
  asymmetry of MR-T1 images of the brain," 2008 5th IEEE International Symposium on Biomedical Imaging: From
  Nano to Macro, Paris, 2008, pp. 424-427. https://doi.org/10.1109/ISBI.2008.4541023
- [Falcao2004] A. X. Falcão and F. P. G. Bergo, "Interactive volume segmentation with
  differential image foresting transforms," in IEEE Transactions on Medical Imaging,
  vol. 23, no. 9, Sep. 2004, pp. 1100-1108. https://doi.org/10.1109/TMI.2004.829335

              

