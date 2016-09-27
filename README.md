# maminfo
Read LTO/Ultrium cartridges' medium auxiliary memory attributes from a tape drive

This software is based on another software for reading/writing the custom text attribute in the
auxiliary memory: https://github.com/scangeo/lto-cm/

All Low-Level SCSI information was taken from the excellent IBM documentation that can be found at
https://www-01.ibm.com/support/docview.wss?uid=ssg1S7003556&aid=1

You will need sg3_utils installed to build this (Package sg3_utils-devel on RedHat'ish systems).
