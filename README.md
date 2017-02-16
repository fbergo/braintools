
Repository of brain imaging tools, mostly developed between 2002 and 2013 at
the Institute of Computing and Neuroimaging Laboratory at Unicamp.

They were originally developed at whatever Linux we used at the labs at the time, and
have been updated to compile on updated and stricter environments (CentOS 6/7, Fedora 20-25).

Directories:

libip      - image processing library used by IVS and Aftervoxel

aftervoxel - 3D segmentation/visualization tool, GTK+-based GUI

ivs        - 3D semi-automatic segmentation tool,  implements the differential IFT.
             The algorithms and this software are described in

             A. X. Falcão and F. P. G. Bergo, "Interactive volume segmentation with
               differential image foresting transforms," in IEEE Transactions on Medical Imaging,
               vol. 23, no. 9, pp. 1100-1108, Sept. 2004.
               https://doi.org/10.1109/TMI.2004.829335

             the ivs directory also contains the dicom2scn conversion tool and a few command line
             utilities (scntool, scn2ana, ana2scn)
              
Most of these tools operate with the SCN 3D image format, which was defined internally at Unicamp at
the time. A description of the format is available as a man page in ivs/man/scn.5

External dependencies:
- Perl 5 (for configuration scripts)
- GTK+ 2.x and its development libraries
- libpng, libjpeg, zlib and its development headers (-devel packages in RedHat-ish distros).


All software in this repostory is licensed under the GPL v2.
Felipe Bergo, February 2017.

