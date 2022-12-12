# Chapter 9 - Using CDLs in Open RV

As discussed previously there are two default places to set ASC CDL properties in the RV node graph. One is as a file CDL on the RVLinearize node in the RVLinearizePipepline and the other is as a look CDL on the RVColor node in the RVLookPipeline. In the case of the RVLinearize node the CDL is applied before linearization occurs whereas in the case of the RVColor node the CDL s applied after linearization and linear color changes.

**To assign a CDL file to the source:**

* The Import menu under the File menu is used to assign CDL files to either the source’s Look or File pipelines.

### 9.1 CDL File Formats


The types of files RV supports right now are Color Decision List (.cdl), Color Correction (.cc) and Color Correction Collection (.ccc) files. Color Correction Collection files can include multiple Color Corrections tagged by ids. We do not support reading the properties by id. Therefore the first Color Collection found in the file will be read and used.