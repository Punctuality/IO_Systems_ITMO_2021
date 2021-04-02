#!/usr/bin/fish

if [ (whoami) != 'root' ]
  sudo './'(basename (status -f)) $argv
  exit $status
end

mkfs.vfat /dev/mydisk1
mkfs.vfat /dev/mydisk2
mkfs.vfat /dev/mydisk5
mkfs.vfat /dev/mydisk6

mkdir /mnt/disk1
mkdir /mnt/disk2
mkdir /mnt/disk5
mkdir /mnt/disk6

mount /dev/mydisk1 /mnt/disk1
mount /dev/mydisk2 /mnt/disk2
mount /dev/mydisk5 /mnt/disk5
mount /dev/mydisk6 /mnt/disk6

function create_files
  dd if=/dev/urandom of=/mnt/disk1/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk2/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk5/file bs=1M count=5
  dd if=/dev/urandom of=/mnt/disk6/file bs=1M count=5
end

function delete_files
  rm /mnt/disk1/file
  rm /mnt/disk2/file
  rm /mnt/disk5/file
  rm /mnt/disk6/file
end

create_files

  echo "Copying files within virtual disk"
  pv /mnt/disk1/file > /mnt/disk6/file
  pv /mnt/disk2/file > /mnt/disk6/file
  pv /mnt/disk5/file > /mnt/disk6/file
  pv /mnt/disk6/file > /mnt/disk1/file

delete_files

create_files
  echo "Copying files from virtual file to real disk"
  mkdir /tmp/io
  pv /mnt/disk1/file > /tmp/io/testfile
  pv /mnt/disk2/file > /tmp/io/testfile
  pv /mnt/disk5/file > /tmp/io/testfile
  pv /mnt/disk6/file > /tmp/io/testfile
delete_files

umount /dev/mydisk1
umount /dev/mydisk2
umount /dev/mydisk5
umount /dev/mydisk6

rm -rf /tmp/io/
rmdir /mnt/disk1
rmdir /mnt/disk2
rmdir /mnt/disk5
rmdir /mnt/disk6