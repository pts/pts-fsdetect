pts-fsdetect: C library for detecting a few filesystems

Supported filesystems: ext2, ext3, ext4, FAT (== VFAT) (including FAT12,
FAT16, FAT32), NTFS, Btrfs.

The library also supports extracting the volume label and the volume UUID
from those filesystems.

The sanity checks in pts-fsdetect are stricter than those in util-linux
or Busybox (i.e. the /sbin/blkid command) and in Syslinux 4.07. Stricter
checks make sure that a block of random junk doesn't get misdetected as a
filesystem.

License: GNU GPL v2 or newer.

__END__
