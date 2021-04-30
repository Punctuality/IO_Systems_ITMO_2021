#!/usr/bin/fish

if [ (whoami) != 'root' ]
  sudo './'(basename (status -f)) $argv
  exit $status
end

if [ $argv[1] == ' ' or $argv[1] == '' ]
  echo "You've typed nothing, so there's a little help for you:"
  ls /dev/ | grep "disk"
else
  fdisk -l -u=cylinders $argv[1]
end
