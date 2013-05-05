#!usr/bin/env python
# The script copies the source code and Makefile.prep in the current directory
# to $LLVM_SRC_ROOT/lib/SpeGen, and build the passes

import os
import shutil

# Arsenal Root Path
arsenal_root = os.path.realpath('../../..')
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
dest_root_dir = os.path.join(llvm_src_root, 'lib', 'SpeGen')
if not os.path.exists(dest_root_dir):
  os.mkdir(dest_root_dir)

dest_dir = os.path.join(llvm_src_root, 'lib', 'SpeGen', src_dir_name)
if not os.path.exists(dest_dir):
  os.mkdir(dest_dir)
print "Dest Dir: " + dest_dir

shutil.rmtree(dest_dir)
shutil.copytree(os.curdir, dest_dir)

# # Current directory file list
# file_list = os.listdir(os.curdir)
# 
# copy_list = []
# # Find source code files
# for file_name in file_list:
#   if file_name.endswith(".cpp") \
#       or file_name.endswith(".h") \
#       or file_name == 'Makefile':
#     print "Source File: " + file_name
#     copy_list.append(file_name)
# 
# for file in copy_list:
#   src_path = os.path.join(src_dir, file)
#   dest_path = os.path.join(dest_dir, file)
#   print "src: " + src_path
#   print "dest: " + dest_path
#   shutil.copy(src_path, dest_path)

# Bulid passes in dest dir
os.chdir(dest_dir)
print 'Current Dir: ' + os.path.realpath(os.curdir)
os.system('make')
