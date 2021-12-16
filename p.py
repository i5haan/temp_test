from ctypes import *
# either
libc = cdll.LoadLibrary("./libmfs.so")
# or
# print(libc.MFS_Init("localhost", 5000))
print(libc.MFS_Shutdown(None))