#!/usr/bin/env python
# The script clean the built passes and remove the files in dest dir

# Arsenal Root Path
import os
import shutil

arsenal_root = os.path.realpath('../../../arsenal')
print "Arsenal Root: " + arsenal_root

# LLVM SRC Root Path
llvm_src_root = os.path.join(arsenal_root, 'Compiler', 'llvm-3.0')
print "LLVM SRC Root: " + llvm_src_root

# Current Directory Name
src_dir = os.path.realpath(os.curdir)
src_dir_name = os.path.basename(src_dir)
print "Source Dir: " + src_dir
print "Current Dir Name: " + src_dir_name

# Destination Directory Path
dest_dir = os.path.join(llvm_src_root, 'lib', 'SpeGen', src_dir_name)
if not os.path.exists(dest_dir):
  os.mkdir(dest_dir)
print "Dest Dir: " + dest_dir

os.chdir(dest_dir)
os.system('make clean')

os.chdir(src_dir)
shutil.rmtree(dest_dir)
