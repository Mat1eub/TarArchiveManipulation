# TarArchiveManipulation

## Prior understanding

A tar file is an *archive* which can contain many other files or even directories, but with that the whole thing is combined in a single archive `archive.tar`. The prupose is to make easier the storage and transfer process. It also retains metadata file like file permissions and else.

This file can be treated just like a normal file, copied to a USB drive, sent by email as an attachment etc...

Tar archives follow a precise structure: 
- **Header block**:  Each file or directory in the archive is described by a 512 byte header block indicating the file name, size, type, permissions, modification time, user ID, group ID, checksum and magic values (*ustar* and version)
- **Data blocks**: Below the header of a file, there is the file's content stored in data block of 512 byte each. If the file's size isn't a multiple of 512, it will paded with null bytes. 

The command `hexedit` gives us a sight of the headers and the content of the files within the archive.

Usage:
```bash
brew install hexedit (for mac users)
hexedit <archive_name>.tar
```

This command can be useful for checking wether or not an archive is invalid (checksum error, magic numbers error or incorrect block alignment). 

The command `tar -tvf` can also be useful to make a list of the content of the archive.
Usage:
```bash
tar -tvf <archive_name>.tar
```

### Where does tar.gz come from ?
Making a tar archive doesn't compress the data. We can compress the data using gzip. The tar command creates the archive, and gzip compresses it, making a .tar.gz file reducing the overall size.
