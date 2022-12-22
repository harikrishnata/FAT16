# FAT16

This program is an implementation of the FAT16 filesystem that can read disk images created on devices using File Allocation Table

The program first asks the name of the FAT16 image which is to be opened(the image has to be in the same folder as the program). If the image is found it prints the boot sector information of the image in the terminal.

The program then asks for the path of the file stored in the FAT16 image which is to be opened. If the file is found it prints the content of the file in the terminal.
