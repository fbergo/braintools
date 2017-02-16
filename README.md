
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
- **spinseseg**
  Tool for measuring spinal cord atrophy in MRI (performs segmentation with automatic tree pruning [Bergo2007]). The
  tool has was described in [Bergo2012], and its usage is mentioned in [Chevis2013] and [Branco2013]

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
  by Tree Pruning", J. Math. Imag. Vision, 29(2), Nov. 2007, pp.141-162. https://doi.org/10.1007/s10851-007-0035-4
- [Bergo2008] F. P. G. Bergo, A. X. Falcão, C. L. Yasuda and F. Cendes, "FCD segmentation using texture
  asymmetry of MR-T1 images of the brain," 2008 5th IEEE International Symposium on Biomedical Imaging: From
  Nano to Macro, Paris, 2008, pp. 424-427. https://doi.org/10.1109/ISBI.2008.4541023
- [Bergo2012] F.P.G. Bergo, M.C. França Jr, C.F. Chevis, F. Cendes, "SpineSeg: A Segmentation and Measurement Tool
  for Evaluation of Spinal Cord Atrophy, CISTI'2012,
  Jun 2012, Madrid, Spain, vol. 2, pp. 400-403. http://ieeexplore.ieee.org/document/6263238/?tp=&arnumber=6263238
- [Branco2013] L. M. T. Branco, M. de Albuquerque, H.M.T. de Andradem F.P.G. Bergo, A. Nucci and M.C. França Jr.,
  "Spinal cord atrophy correlates with disease duration and severity in amyotrophic lateral sclerosis", ALS and FT Deg, 15(1-2), Mar. 2014, pp. 93-97. http://dx.doi.org/10.3109/21678421.2013.852589
- [Chevis2013] C.F. Chevis, C.B. da Silva, A. d'Abreu, I. Lopes-Cendes, F. Cendes, F.P.G. Bergo, M.C. França Jr, "Spinal
  Cord Atrophy Correlates with Disability in Friedreich's Ataxia",  Cerebellum, 12(1), Feb 2013, pp. 43-47. https://doi.org/10.1007/s12311-012-0390-6
- [Falcao2004] A. X. Falcão and F. P. G. Bergo, "Interactive volume segmentation with
  differential image foresting transforms," in IEEE Transactions on Medical Imaging,
  vol. 23(9), Sep. 2004, pp. 1100-1108. https://doi.org/10.1109/TMI.2004.829335

              

